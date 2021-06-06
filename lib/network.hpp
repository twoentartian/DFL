#pragma once

#define USE_LIBBOOST    false
#define USE_LIBUV       true

#if (USE_LIBUV && USE_LIBBOOST) || !(USE_LIBUV || USE_LIBBOOST)
#error choose libboost or libuv innetwork
#endif

// use libuv
#if USE_LIBUV
#include "network-libuv//tcp_server.hpp"
#include "network-libuv/tcp_client.hpp"
#endif

// use libboost
#if USE_LIBBOOST
#include "network-libboost/tcp_server.hpp"
#include "network-libboost/tcp_client.hpp"
#endif

#undef USE_LIBUV
#undef USE_LIBBOOST
