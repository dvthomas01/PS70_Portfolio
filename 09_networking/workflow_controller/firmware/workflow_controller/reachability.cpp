/**
 * @file reachability.cpp
 */

#include "reachability.h"

#include <HTTPClient.h>

#include "config.h"

bool checkInternetReachable204() {
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(kReachabilityUrl)) {
    return false;
  }
  const int code = http.GET();
  http.end();
  return code == 204;
}
