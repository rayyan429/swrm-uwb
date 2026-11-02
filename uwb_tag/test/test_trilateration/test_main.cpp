// Unit tests for trilaterate2D, run on your PC:  pio test -e native
// These fail until trilateration.cpp is filled in; that is expected.
#include <math.h>
#include <unity.h>
#include <trilateration.h>

// 5 m x 4 m field with an anchor in each corner.
static const float ANCHORS[4][2] = {{0, 0}, {5, 0}, {5, 4}, {0, 4}};

static float distance(float x1, float y1, float x2, float y2) {
    return sqrtf((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void setUp() {}
void tearDown() {}

// Perfect distances should give back the exact position.
void test_exact_distances() {
    const float tx = 2.0f, ty = 1.0f;
    float d[4];
    for (int i = 0; i < 4; i++) {
        d[i] = distance(ANCHORS[i][0], ANCHORS[i][1], tx, ty);
    }

    Point2D p;
    TEST_ASSERT_TRUE(trilaterate2D(ANCHORS, d, 4, p));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, tx, p.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, ty, p.y);
}

// TODO: add more tests, e.g.
//  - noisy distances (+/- 0.1 m each) still land within ~0.15 m
//  - fewer than 3 anchors returns false
//  - three anchors in a straight line returns false

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_exact_distances);
    return UNITY_END();
}
