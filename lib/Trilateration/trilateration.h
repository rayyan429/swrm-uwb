// 2D trilateration: anchor positions + measured distances -> tag position.
// Plain C++ with no Arduino dependency, so it runs on the ESP32 and in the
// native unit tests on your PC.
#pragma once

struct Point2D {
    float x;
    float y;
};

// Solves for the tag position using every anchor (least squares), so a 4th
// anchor improves accuracy instead of being ignored.
//
//   anchors : n rows of {x, y} in metres
//   dists   : n measured distances in metres, same order as anchors
//   n       : number of anchors (needs at least 3)
//   out     : filled with the solution on success
//
// Returns false if n < 3 or the anchors are (nearly) in a straight line,
// because then there is no unique answer.
bool trilaterate2D(const float anchors[][2], const float dists[], int n, Point2D &out);
