/*
 *  raylib-line-triangulator -- line triangulation algorithm for raylib
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

#include "raylib.h"
#include "raymath.h"

#define EPSILON 0.00001f

typedef struct TriLine {
	// User values
	Vector2* points;
	int numPoints;
	float thickness;
	bool loop;

	// Calculated values
	Vector2* strip;
	int stripLen;
} TriLine;

//----------------------------------------------------------------------------------
// Functions definition - Utils math
//----------------------------------------------------------------------------------

// Cross product of two points `a` and `b`
float CrossProduct(Vector2 a, Vector2 b) {
	return a.x*b.y - b.x*a.y;
}

// Returns true if lines (`a1` - `a2`) and (`b1` - `b2`) intersect
// Theoretically, works faster if lines do not intersect
// live demo: https://martin-thoma.com/how-to-check-if-two-line-segments-intersect/
bool DoLinesIntersect(Vector2 a1, Vector2 a2, Vector2 b1, Vector2 b2) {
	Vector2 a = Vector2Subtract(a2, a1);
	Vector2 b = Vector2Subtract(b2, b1);
	return (
		(
			// b1 and b2 are on the different sides of line defined by a1 and a2
			(CrossProduct(a, Vector2Subtract(b1, a1)) > 0) ^
			(CrossProduct(a, Vector2Subtract(b2, a1)) > 0)
		) && (
			// a1 and a2 are on the different sides of line defined by b1 and b2
			(CrossProduct(b, Vector2Subtract(a1, b1)) > 0) ^
			(CrossProduct(b, Vector2Subtract(a2, b1)) > 0)
		)
	);
}

// Finds `right` and `left` perpendiculars to vector pointing to `dir`.
// Perpendiculars are scaled to have length `perp_len`.
// All vectors are located at `center`.
void FindPerpendiculars(Vector2 center, Vector2 dir, float perp_len, Vector2* right, Vector2* left) {
	Vector2 vec = Vector2Subtract(dir, center);
	float scale = perp_len / Vector2Length(vec);
	*left  = Vector2Add(center, (Vector2) { -vec.y * scale,  vec.x * scale });
	*right = Vector2Add(center, (Vector2) {  vec.y * scale, -vec.x * scale });
}

//----------------------------------------------------------------------------------
// Functions definition - "low-level"
//----------------------------------------------------------------------------------

// Returns number of strip points needed to triangulate a line
int GetStripLength(int numPoints, bool loop) {
	if (numPoints > 2)
		return 2 * (numPoints + loop);
	else if (numPoints == 2)
		return 4;  // ignore `loop`
	else
		return 0;
}

// Converts line defined by `numPoints` `points` into a triangle strip.
// Triangulation parameters are `thickness` and `loop`.
// Stores the result into an array pointed by `strip`,
// which must be at least `GetStripLength(numPoints, loop)` elements long.
// The resulting array is ready to be drawn with DrawTriangleStrip().
void TriangulateLine(Vector2* points, int numPoints, float thickness, bool loop, Vector2* strip) {
	if (numPoints == 2)
		loop = false;
	else if (numPoints < 2)
		return;

	thickness /= 2.0f;

	Vector2 A, O, B, p1, p2;
	int offset, reverseOrder;

	if (loop) {
		A = points[numPoints - 1];
		O = points[0];
	} else {
		A = points[0];
		O = points[1];
		FindPerpendiculars(A, O, thickness, strip, strip + 1);
	}

	// main loop
	// skip first and last element if loop is false

	for (int i = !loop; i < numPoints - !loop; i++) {
		/* O is points[i], A is previous point and B is the next one.
		 * a = OA, b = OB, s = OX is a bisector.
		 *
		 *         O
		 *         ^
		 *        /|\
		 *      a/ |s\b
		 *      /__|__\
		 *     A   x   B
		 */

		B = points[(i+1) % numPoints];

		Vector2 s;
		Vector2 a = Vector2Subtract(A, O);
		Vector2 b = Vector2Subtract(B, O);

		float len_b = Vector2Length(b);
		float slopeDiff = a.y/a.x - b.y/b.x;

		if (-EPSILON < slopeDiff && slopeDiff < EPSILON) {
			// a and b are (almost) parallel so triangle AOB (almost) doesn't exist
			// imagine A, O and B on the same line -- bisector s will be perpendicular to this line
			// so here we are calculating perpendicular vector

			float s_scale = thickness / len_b;
			s.x = -b.y * s_scale;
			s.y =  b.x * s_scale;
		} else {
			// find bisector using angle bisector theorem
			float sides_len_ratio = Vector2Length(a) / len_b;

			Vector2 AB = Vector2Subtract(B, A);
			Vector2 AX = Vector2Scale(AB, sides_len_ratio / (sides_len_ratio + 1));
			s = Vector2Add(a, AX);

			// scale s depending on thickness
			float len_s = Vector2Length(s);
			float cos_b_s = (b.x*s.x + b.y*s.y) / len_b / len_s;
			float sin_b_s = sqrtf(1 - cos_b_s * cos_b_s);
			float s_scale = thickness / sin_b_s / len_s;
			s = Vector2Scale(s, s_scale);
		}

		p1 = Vector2Add     (O, s);
		p2 = Vector2Subtract(O, s);

		offset = 2*i;
		reverseOrder =  // strip points order is important!
			// line between consequent points must cross the original line
			( i && DoLinesIntersect(A, O, strip[offset-2], p1)) ||
		  // the very first point must be on the left side relative to vector b
			(!i && CrossProduct(b, Vector2Subtract(points[numPoints-1], O)) > 0);

		strip[offset +  reverseOrder] = p1;
		strip[offset + !reverseOrder] = p2;

		A = O; O = B;
	}

	offset = GetStripLength(numPoints, loop) - 2;

	if (loop) {
		strip[offset + 0] = strip[0];
		strip[offset + 1] = strip[1];
	} else {
		FindPerpendiculars(points[numPoints-1], points[numPoints-2], thickness, &p1, &p2);
		reverseOrder = DoLinesIntersect(A, O, strip[offset-2], p1);

		strip[offset +  reverseOrder] = p1;
		strip[offset + !reverseOrder] = p2;
	}
}

//----------------------------------------------------------------------------------
// Functions definition - "high-level" (using struct)
//----------------------------------------------------------------------------------

// Perform triangulation
// Call when changes are made (including initialization)
void UpdateTriLine(TriLine* triline) {
	int prevStripLen = triline->stripLen;
	triline->stripLen = GetStripLength(triline->numPoints, triline->loop);

	if (triline->stripLen > prevStripLen) {
		// realloc may perform copying
		RL_FREE(triline->strip);
		triline->strip = RL_MALLOC(triline->stripLen * sizeof(Vector2));
	}

	if (triline->stripLen) {
		TriangulateLine(
			triline->points, triline->numPoints, triline->thickness, triline->loop, triline->strip);
	}
}

// Draw triangle strip of a given color
void DrawTriLine(TriLine triline, Color color) {
	DrawTriangleStrip(triline.strip, triline.stripLen, color);
}