/*
 *  raylib-line-triangulator demo -- line triangulation algorithm visualizer
 *
 *  LICENSE: zlib
 *
 *  Copyright (c) 2020 Taras Radchenko (@petuzk)
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 *
 */

#include <stdlib.h>
#include "line_triangulator.h"
#include "raylib.h"
#include "rlgl.h"

void LoadDefaultLine(TriLine* triline, float width, float height) {
	triline->numPoints = 6;

	float xOffset = width / 2.0f, yOffset = height / 2.0f;
	float scale = 0.5f * (width / height < 0.83f ? width : height);

	triline->points[0] = (Vector2) { xOffset + scale * (-0.403f), yOffset + scale * (-0.355f) };
	triline->points[1] = (Vector2) { xOffset + scale * (-0.500f), yOffset + scale * ( 0.161f) };
	triline->points[2] = (Vector2) { xOffset + scale * (-0.226f), yOffset + scale * ( 0.421f) };
	triline->points[3] = (Vector2) { xOffset + scale * ( 0.290f), yOffset + scale * ( 0.338f) };
	triline->points[4] = (Vector2) { xOffset + scale * ( 0.500f), yOffset + scale * (-0.057f) };
	triline->points[5] = (Vector2) { xOffset + scale * ( 0.097f), yOffset + scale * (-0.420f) };
}

int main() {
	// Initialize window
	int screenWidth = 1000;
	int screenHeight = 600;
	int halfScreen = screenWidth / 2;

	SetTraceLogLevel(LOG_WARNING);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, "raylib - line triangulator");

	// Allocate points array and create TriLine

	int numPointsAllocated = 64;
	bool updTriLine = true;

	TriLine triline = { 0 };
	triline.points = malloc(numPointsAllocated * sizeof(Vector2));
	// Note: triline.numPoints is set in LoadDefaultLine()
	triline.loop = true;
	triline.thickness = 16.0f;
	LoadDefaultLine(&triline, halfScreen, screenHeight);

	SetTargetFPS(30);  // There are no animations so no need to be 60

	while (!WindowShouldClose()) {

		// Process events

		bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

		#if defined(__APPLE__)
			bool ctrl = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
		#else
			bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
		#endif

		if (IsWindowResized()) {
			screenWidth = GetScreenWidth();
			screenHeight = GetScreenHeight();
			halfScreen = screenWidth / 2;
		}

		if (shift && IsKeyPressed(KEY_R)) {  // Reset thickness
			triline.thickness = 16.0f;
			updTriLine = true;
		}

		else if (IsKeyPressed(KEY_R)) {  // Reset line
			LoadDefaultLine(&triline, halfScreen, screenHeight);
			updTriLine = true;
		}

		else if (IsKeyPressed(KEY_C)) {  // Clear line
			triline.numPoints = 0;
			updTriLine = true;
		}

		else if (IsKeyPressed(KEY_L)) {  // Toggle loop
			triline.loop = !triline.loop;
			updTriLine = true;
		}

		else if (IsKeyDown(KEY_DOWN)) {  // Decrease thickness
			triline.thickness -= 0.4f + shift*0.8f;
			updTriLine = true;
		}

		else if (IsKeyDown(KEY_UP)) {  // Increase thickness
			triline.thickness += 0.4f + shift*0.8f;
			updTriLine = true;
		}

		else if (ctrl && IsKeyPressed(KEY_Z) && triline.numPoints) {  // Remove last point
			triline.numPoints--;
			updTriLine = true;
		}

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			if (triline.numPoints == numPointsAllocated) {
				triline.points = realloc(triline.points, (numPointsAllocated += 16) * sizeof(Vector2));
			}

			Vector2 pos = GetMousePosition();
			if (pos.x >= halfScreen) {
				pos.x -= halfScreen;
			}

			triline.points[triline.numPoints++] = pos;
			updTriLine = true;
		}

		// Update TriLine if needed

		if (updTriLine) {
			UpdateTriLine(&triline);
		}

		// Draw

		BeginDrawing();

			ClearBackground(RAYWHITE);

			// Left side

			BeginScissorMode(0, 0, halfScreen, screenHeight);

				// Draw line and points

				DrawLineStrip(triline.points, triline.numPoints, RED);

				for (int i = 0; i < triline.numPoints; i++) {
					DrawRectangle(triline.points[i].x-3, triline.points[i].y-3, 6, 6, RED);
				}

				// Draw strip and points changing color from green to blue and decreasing size

				DrawLineStrip(triline.strip, triline.stripLen, BLACK);

				for (int i = 0; i < triline.stripLen; i++) {
					int c = 255.0f * i / triline.stripLen;
					int size = 8.0f - 4.0f * i / triline.stripLen;

					DrawRectangle(
						triline.strip[i].x - size/2, triline.strip[i].y - size/2,
						size, size,
						(Color) { 0, 255-c, c, 255 }
					);
				}

				DrawText("click to add point, ctrl+Z to remove last one", 10, 10, 10, BLACK);
				DrawText("C to clear line, R to reset line", 10, 30, 10, BLACK);
				DrawText("up/down to change thickness, hold shift to speed up", 10, 50, 10, BLACK);
				DrawText("L to toggle loop, shift+R to reset thickness", 10, 70, 10, BLACK);

			EndScissorMode();

			// Right side

			BeginScissorMode(halfScreen, 0, halfScreen, screenHeight);
			rlPushMatrix();

				// Use the same coordinates for drawing on the right side
				rlTranslatef(halfScreen, 0, 0);

				DrawTriLine(triline, (Color) {40, 40, 40, 255});
				DrawLineStrip(triline.strip, triline.stripLen, BLACK);
				DrawLineStrip(triline.points, triline.numPoints, RED);

				DrawText(TextFormat("frames: %d", GetFPS()), 10, 10, 10, BLACK);
				DrawText(TextFormat("thickness: %.1f", triline.thickness), 10, 30, 10, BLACK);
				DrawText(TextFormat("loop: %s", triline.loop ? "on" : "off"), 10, 50, 10, BLACK);
				DrawText(TextFormat("points: %d", triline.numPoints), 10, 70, 10, BLACK);

			rlPopMatrix();
			EndScissorMode();

			// Separator

			DrawRectangle(halfScreen - 2, 0, 4, screenHeight, BLACK);

		EndDrawing();
	}

	CloseWindow();

	return 0;
}