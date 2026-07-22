#include <math.h>    // Required for: atan2f(), cosf(), sinf(), fminf().
#include <stddef.h>  // Required for: NULL.
#include <stdlib.h>  // Required for: EXIT_FAILURE, EXIT_SUCCESS.
#include <string.h>  // Required for: strcmp().
#include <stdbool.h> // Required for: bool, true, false.

#include "raylib.h"

#define RAYTMX_IMPLEMENTATION
#include "raytmx.h"

// Collision is checked against the "collision" object group defined in the TMX file (no Tile Collision Editor,
// no doors/portals group -- this map only has the one object group used for walls/collisions).
#define CHECK_COLLISION_OBJECT_GROUP true
#define COLLISION_OBJECT_GROUP_NAME "collision"

// Extra margin (in pixels) left around the map when it's fit inside the window, so it doesn't touch the edges.
#define FIT_MARGIN_PIXELS 10.0f

// Multiplier applied on top of the "fit the whole map" zoom, to make the map/frame appear bigger. 1.0 = exact fit
// (no cropping). Values above 1.0 zoom in a bit more, which may crop a small amount off the map's edges since an
// explicit whole-map viewport is used for drawing (nothing breaks, it just won't all be visible on screen).
#define ZOOM_BOOST 1.35f

#define PLAYER_SPEED_IN_PIXELS_PER_SECOND 80.0f
#define POLYGON_POINTS_COUNT 6

typedef struct Polygon {
    Vector2 center;
    float radius;
    Vector2 points[POLYGON_POINTS_COUNT];
    Rectangle aabb;
} Polygon;

// Structure to hold depth-sorted render data
typedef struct DepthObject {
    TmxObject *tmxObject;
    float bottomY;
    bool isBehindPlayer;
} DepthObject;

// Get a Polygon with a given center point and radius.
static Polygon GetPolygon(Vector2 center, float radius)
{
    Polygon poly = { ZERO_INIT };
    poly.center = center;
    poly.radius = radius;
    poly.aabb.x = center.x - radius;
    poly.aabb.y = center.y - radius;
    poly.aabb.width = 2.0f*radius;
    poly.aabb.height = 2.0f*radius;

    float theta = 0.0f;
    const float dTheta = 2.0f*PI/(float)POLYGON_POINTS_COUNT;
    for (int i = 0; i < POLYGON_POINTS_COUNT; i++)
    {
        poly.points[i].x = center.x + (radius*cosf(theta));
        poly.points[i].y = center.y + (radius*sinf(theta));
        theta += dTheta;
    }

    return poly;
}

// Get a Polygon derived from a given one with the given deltas.
static Polygon TranslatePolygon(Polygon poly, float dx, float dy)
{
    // Translate the center point.
    poly.center.x += dx;
    poly.center.y += dy;

    // Translate each vertex the same amounts.
    for (int i = 0; i < POLYGON_POINTS_COUNT; i++)
    {
        poly.points[i].x += dx;
        poly.points[i].y += dy;
    }

    // Translate the Axis-Aligned Bounding Box (AABB).
    poly.aabb.x += dx;
    poly.aabb.y += dy;

    return poly;
}

// Comparison function for sorting depth objects by bottom Y coordinate (ascending)
static int CompareDepthObjects(const void *a, const void *b)
{
    const DepthObject *objA = (const DepthObject *)a;
    const DepthObject *objB = (const DepthObject *)b;
    
    if (objA->bottomY < objB->bottomY) return -1;
    if (objA->bottomY > objB->bottomY) return 1;
    return 0;
}

// Draw a single object with proper depth rendering
static void DrawDepthObject(TmxObject *obj, Color color)
{
    // Draw the object as a rectangle with the given color
    DrawRectangle((float)obj->x, (float)obj->y, (float)obj->width, (float)obj->height, color);
}

int main(void)
{
    // This map makes use of many TMX features making it useful for demonstrations.
    const char *fileName = "map.tmx";
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);

    InitWindow(monitorWidth, monitorHeight, "raytmx collisions example");
    ToggleFullscreen();   // Enter fullscreen

    // Use the ACTUAL current screen size (post-fullscreen), not the pre-window monitor query, since the two
    // can differ depending on platform/OS.
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    SetTargetFPS(60);
    // Load the map. If loading fails, NULL will be returned and details will be TraceLog()'d.
    TmxMap *map = LoadTMX(fileName);
    if (map == NULL)
    {
        TraceLog(LOG_ERROR, "Failed to load TMX \"%s\"", fileName);
        CloseWindow();
        return EXIT_FAILURE;
    }

    // Full size of the map, in pixels.
    const float mapPixelWidth = (float)(map->width*map->tileWidth);
    const float mapPixelHeight = (float)(map->height*map->tileHeight);

    // An explicit viewport covering the WHOLE map. Passing this to DrawTMX (instead of NULL) guarantees every
    // tile is considered for drawing, regardless of how camera.zoom would otherwise make the library derive a
    // (possibly broken, at extreme zoom-out) viewport on its own.
    const Rectangle mapViewport = { 0.0f, 0.0f, mapPixelWidth, mapPixelHeight };

    // Calculate the map's center point. It's used to position the camera and the "player."
    const Vector2 mapCenter = { mapPixelWidth/2.0f, mapPixelHeight/2.0f };

    // Create a camera. Cameras use matrices to efficiently look at select parts of the map/world.
    Camera2D camera = { ZERO_INIT };
    camera.offset.x = (float)screenWidth/2.0f;
    camera.offset.y = (float)screenHeight/2.0f;
    camera.target = mapCenter;
    camera.rotation = 0.0f;

    // Zoom out just enough so that the ENTIRE map fits inside the window, on both axes.
    const float zoomX = ((float)screenWidth - FIT_MARGIN_PIXELS)/mapPixelWidth;
    const float zoomY = ((float)screenHeight - FIT_MARGIN_PIXELS)/mapPixelHeight;
    camera.zoom = fminf(zoomX, zoomY)*ZOOM_BOOST;
    if (camera.zoom <= 0.0f) camera.zoom = 1.0f; // Safety fallback; should not normally trigger.

    TraceLog(LOG_INFO, "screen: %dx%d | map: %.0fx%.0f px | zoom: %.4f", screenWidth, screenHeight, mapPixelWidth,
        mapPixelHeight, camera.zoom);

    // Loop through the layers to find the "collision" object group that determines where we can and cannot go.
    TmxObjectGroup wallsObjectGroup = { ZERO_INIT };
    for (size_t i = 0; i < map->layersLength; i++)
    {
        TmxLayer layer = map->layers[i];

        if ((strcmp(layer.name, COLLISION_OBJECT_GROUP_NAME) == 0) && (layer.type == LAYER_TYPE_OBJECT_GROUP))
        {
            wallsObjectGroup = layer.exact.objectGroup;
            break;
        }
    }

    // The "player" always spawns at the center of the map, and its size is scaled by the fit zoom so that it
    // stays a sensible, visible size on screen instead of ballooning past the frame.
    Polygon poly = GetPolygon(mapCenter, ((float)map->tileWidth/3.0f));

    // Allocate memory for depth objects
    DepthObject *depthObjects = NULL;
    size_t depthObjectCount = 0;
    
    // Collect all objects from the collision group
    if (wallsObjectGroup.objectsLength > 0)
    {
        depthObjects = (DepthObject *)malloc(wallsObjectGroup.objectsLength * sizeof(DepthObject));
        if (depthObjects != NULL)
        {
            for (size_t i = 0; i < wallsObjectGroup.objectsLength; i++)
            {
                TmxObject *obj = &wallsObjectGroup.objects[i];
                depthObjects[depthObjectCount].tmxObject = obj;
                depthObjects[depthObjectCount].bottomY = (float)(obj->y + obj->height);
                depthObjects[depthObjectCount].isBehindPlayer = false;
                depthObjectCount++;
            }
        }
    }

    while (!WindowShouldClose())
    {
        // If one or more arrow key is pressed.
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_UP))
        {
            // Create a velocity vector with a magnitude equal to the player's allowed speed.
            Vector2 velocity = { 0.0f, 0.0f };
            if (IsKeyDown(KEY_RIGHT)) velocity.x += 1.0f;
            if (IsKeyDown(KEY_LEFT)) velocity.x -= 1.0f;
            if (IsKeyDown(KEY_DOWN)) velocity.y += 1.0f;
            if (IsKeyDown(KEY_UP)) velocity.y -= 1.0f;
            const float theta = atan2f(velocity.y, velocity.x); // Angle of the vector in radians.
            velocity.x = cosf(theta)*PLAYER_SPEED_IN_PIXELS_PER_SECOND*GetFrameTime(); // X component.
            velocity.y = sinf(theta)*PLAYER_SPEED_IN_PIXELS_PER_SECOND*GetFrameTime(); // Y component.

            // Translate the player one axis at a time. If this movement leads the player to hit a wall, revert the
            // player's position for just that axis.
            poly = TranslatePolygon(poly, velocity.x, 0.0f);

#if CHECK_COLLISION_OBJECT_GROUP
            if (CheckCollisionTMXObjectGroupPoly(wallsObjectGroup, poly.points, POLYGON_POINTS_COUNT, NULL))
#else
            if (CheckCollisionTMXTileLayersPolyEx(map, map->layers, map->layersLength, poly.points,
                POLYGON_POINTS_COUNT, poly.aabb, NULL))
#endif
            {
                poly = TranslatePolygon(poly, -velocity.x, 0.0f); // Undo the X translation.
            }

            poly = TranslatePolygon(poly, 0.0f, velocity.y);
#if CHECK_COLLISION_OBJECT_GROUP
            if (CheckCollisionTMXObjectGroupPoly(wallsObjectGroup, poly.points, POLYGON_POINTS_COUNT, NULL))
#else
            if (CheckCollisionTMXTileLayersPolyEx(map, map->layers, map->layersLength, poly.points,
                POLYGON_POINTS_COUNT, poly.aabb, NULL))
#endif
            {
                poly = TranslatePolygon(poly, 0.0f, -velocity.y); // Undo the Y translation.
            }
        }

        // Calculate player feet position
        float playerFeetY = poly.center.y + poly.radius;

        // Determine which objects should be behind or in front of the player
        if (depthObjects != NULL && depthObjectCount > 0)
        {
            for (size_t i = 0; i < depthObjectCount; i++)
            {
                // If player feet are above the bottom of the object, render object behind player
                // If player feet are below, render object in front of player
                depthObjects[i].isBehindPlayer = (playerFeetY < depthObjects[i].bottomY);
            }
            
            // Sort objects by their bottom Y coordinate for proper layering
            // Objects with lower bottomY should be rendered first (behind)
            // Objects with higher bottomY should be rendered later (in front)
            qsort(depthObjects, depthObjectCount, sizeof(DepthObject), CompareDepthObjects);
        }

        // Keep the camera fixed on the map's center so the whole map always stays fitted and centered in view.
        // (If you'd rather have the camera follow the player instead, use: camera.target = poly.center;)
        camera.target = mapCenter;

        BeginDrawing();
        {
            ClearBackground(BLACK);
            BeginMode2D(camera);
            {
                // Update animated tiles to new frames if enough time has passed.
                AnimateTMX(map);
                
                // Draw ONLY the tile layers first (ground, walls, etc.)
                // Skip the object group layer since we'll draw objects manually with depth sorting
                for (size_t i = 0; i < map->layersLength; i++)
                {
                    TmxLayer layer = map->layers[i];
                    // Skip object group layers - we'll draw them manually
                    if (layer.type != LAYER_TYPE_OBJECT_GROUP)
                    {
                        DrawTMXLayers(map, &camera, &mapViewport, &layer, 1, 0, 0, WHITE);
                    }
                }
                
                // Draw objects that should appear BEHIND the player (lower bottomY)
                if (depthObjects != NULL)
                {
                    for (size_t i = 0; i < depthObjectCount; i++)
                    {
                        if (depthObjects[i].isBehindPlayer)
                        {
                            TmxObject *obj = depthObjects[i].tmxObject;
                            // Draw the object in red (behind player)
                            DrawRectangle((float)obj->x, (float)obj->y, (float)obj->width, (float)obj->height, 
                                RED);
                        }
                    }
                }
                
                // Draw the "player" in the middle
                DrawPoly(poly.center, POLYGON_POINTS_COUNT, poly.radius, 0.0f, DARKBLUE);
                
                // Draw objects that should appear IN FRONT of the player (higher bottomY)
                if (depthObjects != NULL)
                {
                    for (size_t i = 0; i < depthObjectCount; i++)
                    {
                        if (!depthObjects[i].isBehindPlayer)
                        {
                            TmxObject *obj = depthObjects[i].tmxObject;
                            // Draw the object in blue (in front of player)
                            DrawRectangle((float)obj->x, (float)obj->y, (float)obj->width, (float)obj->height, 
                                BLUE);
                        }
                    }
                }
            }
            EndMode2D();
            
            // Display debug information
            DrawFPS(10, 10);
            DrawText(TextFormat("Objects: %d", depthObjectCount), 10, 40, 20, WHITE);
            DrawText(TextFormat("Player feet Y: %.1f", poly.center.y + poly.radius), 10, 65, 20, WHITE);
            DrawText("Red: Behind player | Blue: In front of player", 10, 90, 20, WHITE);
        }
        EndDrawing();
    }

    // Clean up
    if (depthObjects != NULL)
    {
        free(depthObjects);
        depthObjects = NULL;
    }

    UnloadTMX(map);
    CloseWindow();

    return EXIT_SUCCESS;
}