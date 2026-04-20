/**
 * @file reachability.h
 * @brief HTTP reachability probe (expects HTTP 204).
 */
#pragma once

/**
 * @return true if GET to kReachabilityUrl returns HTTP 204.
 */
bool checkInternetReachable204();
