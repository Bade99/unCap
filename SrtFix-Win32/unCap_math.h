#pragma once

//int
static int safe_ratioN(int dividend, int divisor, int n) {
	int res;
	if (divisor != 0) {
		res = dividend / divisor;
	}
	else {
		res = n;
	}
	return res;
}

static int safe_ratio1(int dividend, int divisor) {
	return safe_ratioN(dividend, divisor, 1);
}

static int safe_ratio0(int dividend, int divisor) {
	return safe_ratioN(dividend, divisor, 0);
}

static int distance(int a, int b) {
	return abs(a - b);
}

static int clamp(int min, int n, int max) { //clamps between [min,max]
	int res = n;
	if (res < min) res = min;
	else if (res > max) res = max;
	return res;
}

//float
static float safe_ratioN(float dividend, float divisor, float n) {
	float res;
	if (divisor != 0.f) {
		res = dividend / divisor;
	}
	else {
		res = n;
	}
	return res;
}

static float safe_ratio1(float dividend, float divisor) {
	return safe_ratioN(dividend, divisor, 1.f);
}

static float safe_ratio0(float dividend, float divisor) {
	return safe_ratioN(dividend, divisor, 0.f);
}