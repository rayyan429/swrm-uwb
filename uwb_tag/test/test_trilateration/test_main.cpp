// Unit tests for trilaterate2D, run on your PC:  pio test -e native
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
    const float tx = 2.0f, ty = 1.0f; // tag's true x and true y 
    float d[4];
    for (int i = 0; i < 4; i++) {
        d[i] = distance(ANCHORS[i][0], ANCHORS[i][1], tx, ty);
    }

    Point2D p;
    TEST_ASSERT_TRUE(trilaterate2D(ANCHORS, d, 4, p));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, tx, p.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, ty, p.y);
}

void test_noisy_distances() {
    const float tx = 2.0f, ty = 1.0f;
    const float noise[4] = {0.1f, -0.1f, 0.1f, -0.1f};
    float d[4]; // d[0] is distance to anchor 0 at (0,0)
    for (int i = 0; i < 4; i++) {
        d[i] = distance(ANCHORS[i][0], ANCHORS[i][1], tx, ty) + noise[i];
    }

    Point2D p;
    TEST_ASSERT_TRUE(trilaterate2D(ANCHORS, d, 4, p));
    TEST_ASSERT_FLOAT_WITHIN(0.15f, tx, p.x);
    TEST_ASSERT_FLOAT_WITHIN(0.15f, ty, p.y);
}

void test_too_few_anchors(){
    const float tx = 20.0f, ty = 10.0f;
    float d[2];
    for (int i = 0; i < 2; i++) {
        d[i] = distance(ANCHORS[i][0], ANCHORS[i][1], tx, ty);
    }
    Point2D p;
    TEST_ASSERT_FALSE(trilaterate2D(ANCHORS, d, 2, p));
}

void test_colinear_anchors(){  // anchors passing thru the same line can cause confusion especially with mirror image bc distances are the same. 
                                // This can obscure the actual position of the tag.
    const float tx= 2.0f, ty = 1.0f;
    float d[3];
    const float anchors[3][2] = {{0, 0}, {5, 0}, {10, 0}}; // Colinear anchors
    for (int i = 0; i < 3; i++) {
        d[i] = distance(anchors[i][0], anchors[i][1], tx, ty);
    }
    Point2D p;
    TEST_ASSERT_FALSE(trilaterate2D(anchors, d, 3, p));
}


int main() {
    UNITY_BEGIN();
    RUN_TEST(test_exact_distances);
    RUN_TEST(test_noisy_distances);
    RUN_TEST(test_too_few_anchors);
    RUN_TEST(test_colinear_anchors);
    return UNITY_END();
}
