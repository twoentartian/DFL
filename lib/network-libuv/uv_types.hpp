#pragma once

namespace network
{
	using BufferMode = uv::GlobalConfig::BufferMode;
	using EventLoop = uv::EventLoop;
	using SocketAddr = uv::SocketAddr;
	using DefaultCallback = uv::DefaultCallback;
	using AfterWriteCallback = uv::AfterWriteCallback;
	using TcpConnectionPtr = uv::TcpConnectionPtr;
	using SocketAddr = uv::SocketAddr;
	
	using OnCloseCallback = uv::OnCloseCallback;
	using CloseCompleteCallback = uv::OnCloseCallback;
	using OnConnectionStatusCallback = uv::OnConnectionStatusCallback;
	using OnMessageCallback = uv::OnMessageCallback;
	
	
}