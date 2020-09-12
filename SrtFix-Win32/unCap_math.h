#pragma once

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

int distance(int a, int b) {
	return abs(a - b);
}