/**
 * @file ws_transport.h
 * @brief WebSocket client to the macOS host.
 */
#pragma once

void wsTransportBegin();
void wsTransportLoop();

bool wsIsConnected();

bool wsSendChord(const char* chord);
bool wsSendPotentiometer(int value);
