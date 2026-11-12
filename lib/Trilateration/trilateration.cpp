#include "trilateration.h"
#include <cmath>

// The math
// --------
// Each anchor i at (xi, yi) with measured distance di gives a circle:
//
//     (x - xi)^2 + (y - yi)^2 = di^2
//
// These are not linear in x and y (the squares). Subtract anchor 0's
// equation from anchor i's and the x^2 and y^2 terms cancel, leaving one
// linear equation per extra anchor (i = 1 .. n-1):
//
//     2(xi - x0) x + 2(yi - y0) y = d0^2 - di^2 + xi^2 - x0^2 + yi^2 - y0^2
//     \_________/   \_________/   \_____________________________________/
//         a_i1          a_i2                        b_i
//
// That is A p = b, with A an (n-1) x 2 matrix and p = (x, y).
// With 4 anchors you get 3 equations for 2 unknowns, and noisy distances
// mean no point satisfies all of them. Least squares picks the p that
// minimizes the total squared error by solving the "normal equations":
//
//     (A^T A) p = A^T b, p are the unknown xy pairs 
//
// A^T A is just 2x2, so you can solve it directly:
//
//     A^T A = | Sxx  Sxy |      A^T b = | Sxb |
//             | Sxy  Syy |              | Syb |
//
//     where Sxx = sum(a_i1^2), Sxy = sum(a_i1 * a_i2), Syy = sum(a_i2^2), THESE ARE THE SUMS OF ANCHORS
//           Sxb = sum(a_i1 * b_i), Syb = sum(a_i2 * b_i)
//
//     det = Sxx * Syy - Sxy^2
//     x = (Syy * Sxb - Sxy * Syb) / det
//     y = (Sxx * Syb - Sxy * Sxb) / det
//
// If det is ~0 the anchors are colinear and there is no unique answer.

bool trilaterate2D(const float anchors[][2], const float dists[], int n, Point2D &out) {
    if (n < 3) {
        return false;
    }
    const float x0 = anchors[0][0], y0 = anchors[0][1], d0 = dists[0];
    float Sxx = 0, Sxy = 0, Syy = 0, Sxb = 0, Syb = 0;

    for (int i = 1; i < n; i++) {
        const float xi = anchors[i][0], yi = anchors[i][1], di = dists[i];
        const float a_i1 = 2 * (xi - x0);
        const float a_i2 = 2 * (yi - y0);
        const float b_i = d0 * d0 - di * di + xi * xi - x0 * x0 + yi * yi - y0 * y0;

        Sxx += a_i1 * a_i1;
        Sxy += a_i1 * a_i2;
        Syy += a_i2 * a_i2;
        Sxb += a_i1 * b_i;
        Syb += a_i2 * b_i;
    }

    const float det = Sxx * Syy - Sxy * Sxy;
    if (fabsf(det) < 1e-6f) {
        return false;
    }

    const float x = (Syy * Sxb - Sxy * Syb) / det;
    const float y = (Sxx * Syb - Sxy * Sxb) / det;
    out.x = x;
    out.y = y;
    return true;
}
