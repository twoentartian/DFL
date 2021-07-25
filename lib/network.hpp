#pragma once

#if !defined(NETWORK_USE_LIBBOOST) && !defined(NETWORK_USE_LIBUV)
#define NETWORK_USE_LIBBOOST    true
#define NETWORK_USE_LIBUV       false
#else
#ifdef NETWORK_USE_LIBBOOST
#undef NETWORK_USE_LIBBOOST
#define NETWORK_USE_LIBBOOST true
#endif
#ifdef NETWORK_USE_LIBUV
#undef NETWORK_USE_LIBUV
#define NETWORK_USE_LIBUV true
#endif
#endif

#if (NETWORK_USE_LIBUV && NETWORK_USE_LIBBOOST) || !(NETWORK_USE_LIBUV || NETWORK_USE_LIBBOOST)
#error choose libboost or libuv innetwork
#endif

// use libuv
#if NETWORK_USE_LIBUV
#error LIBUV has been removed from this project
#include "network-libuv/tcp_server.hpp"
#include "network-libuv/tcp_client.hpp"
#include "network-libuv/p2p.hpp"
#endif

// use libboost
#if NETWORK_USE_LIBBOOST
#include "network-libboost/p2p.hpp"
#include "network-libboost/udp.hpp"
#include "network-libboost/tcp_simple_server.hpp"
#include "network-libboost/tcp_simple_client.hpp"
#include "network-libboost/tcp_simple_server_with_header.hpp"
#include "network-libboost/tcp_simple_client_with_header.hpp"
#include "network-libboost/p2p_with_header.hpp"
#endif

#undef NETWORK_USE_LIBUV
#undef NETWORK_USE_LIBBOOST
