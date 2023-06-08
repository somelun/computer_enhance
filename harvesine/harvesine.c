#define double f64

static f64 square(f64 a) {
  f64 result = (a * a);
  return result;
}

static f64 degrees_to_radians(f64 degrees) {
  f64 result = 0.01745329251994329577 * degrees;
  return result;
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
static f64 reference_haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius) {
  // NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
  //  Instead, it attempts to follow, as closely as possible, the formula used in the real-world
  //  question on which these homework exercises are loosely based.
  //

  f64 lat1 = y0;
  f64 lat2 = y1;
  f64 lon1 = x0;
  f64 lon2 = x1;

  f64 dLat = degrees_to_radians(lat2 - lat1);
  f64 dLon = degrees_to_radians(lon2 - lon1);
  lat1 = degrees_to_radians(lat1);
  lat2 = degrees_to_radians(lat2);

  f64 a = square(sin(dLat / 2.0)) + cos(lat1) * cos(lat2) * square(sin(dLon / 2));
  f64 c = 2.0 * asin(sqrt(a));

  f64 result = earthr_adius * c;

  return result;
}

