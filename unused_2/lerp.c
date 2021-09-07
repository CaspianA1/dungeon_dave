typedef struct {
	const double x0, y0, slope;
} Lerp; // linear interpolation

Lerp init_lerp(const double x0, const double x1, const double y0, const double y1) {
	const double slope = (y1 - y0) / (x1 - x0);
	DEBUG(slope, lf);
	return (Lerp) {x0, y0, slope};
}

double interpolate(const Lerp lerp, const double x) {
	return lerp.y0 + (x - lerp.x0) * lerp.slope;
}
