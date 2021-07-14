#pragma once

#include <uv/include/uv11.hpp>

namespace network
{
	using BufferMode = uv::GlobalConfig::BufferMode;
	using EventLoop = uv::EventLoop;
	using SocketAddr = uv::SocketAddr;
	using DefaultCallback = uv::DefaultCallback;
	using TcpConnectionPtr = uv::TcpConnectionPtr;
	using SocketAddr = uv::SocketAddr;
	using PacketBufferPtr = uv::PacketBufferPtr;
	using AfterWriteCallback = uv::AfterWriteCallback;
	using WriteInfo = uv::WriteInfo;
}