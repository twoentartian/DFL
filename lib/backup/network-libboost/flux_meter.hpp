#pragma once

#include <memory>
#include <unordered_map>
#include <random>
#include <thread>
#include <atomic>

#include <lock.hpp>

class flux_meter : public std::enable_shared_from_this<flux_meter>
{
public:
	flux_meter(bool use = true)
	{
		++_useCounter;
		
		if (!use)
		{
			_use = use;
			return;
		}
		
		//Instance Init
		if (_instance == nullptr)
		{
			static flux_meter tempMeter(false);
			_instance = &tempMeter;
		}
		
		_limit = 0;
		_currentFluxRate = 0;
		_currentFluxCount = 0;
		_isThisMeterRunning = false;
		
		//Init refreshThread
		if (!_refreshThread)
		{
			_systemRunning = true;
			std::thread tempThread([this]()
			                       {
				                       time_t now = time(nullptr);
				                       while (true)
				                       {
					                       if (!_systemRunning)
					                       {
						                       break;
					                       }
					
					                       for (auto&& flux_meter_iter : _globalMeterContainer)
					                       {
						                       auto& fluxMeter = *flux_meter_iter.second;
						                       fluxMeter._currentFluxRate = fluxMeter._currentFluxCount;
						                       fluxMeter._currentFluxCount = 0;
					                       }
					
					                       while (time(nullptr) == now)
					                       {
						                       std::this_thread::sleep_for(std::chrono::milliseconds(1));
					                       }
					                       now = time(nullptr);
				                       }
			                       });
			_refreshThread.reset(new std::thread);
			_refreshThread->swap(tempThread);
		}
	}
	
	~flux_meter()
	{
		--_useCounter;
		if (_useCounter == 0)
		{
			ShutdownService();
		}
	}
	
	void Start()
	{
		if (_isThisMeterRunning)
		{
			return;
		}
		else
		{
			_isThisMeterRunning = true;
		}
		
		//Add to flux meter container
		static std::random_device rd;
		static std::default_random_engine random_engine(rd());
		static std::uniform_int_distribution<uint32_t> range(0, std::numeric_limits<uint32_t>::max());
		uint32_t random;
		_lockGlobalMeterContainer.LockWrite();
		while (true)
		{
			random = range(random_engine);
			
			const auto result = _globalMeterContainer.find(random);
			if (result == _globalMeterContainer.end())
			{
				break;
			}
		}
		
		_id = random;
		auto self(shared_from_this());
		_globalMeterContainer.insert(std::make_pair(random, self));
		_lockGlobalMeterContainer.UnlockWrite();
	}
	
	bool IsUsed() const
	{
		return _use;
	}
	
	bool IsRunning() const
	{
		return _isThisMeterRunning;
	}
	
	uint64_t GetCurrentFluxRate() const
	{
		return _currentFluxRate;
	}
	
	void PassFlux(uint64_t flux)
	{
		_currentFluxCount += flux;
	}
	
	void StopMeter()
	{
		if (_isThisMeterRunning)
		{
			_isThisMeterRunning = false;
		}
		else
		{
			return;
		}
		
		_lockGlobalMeterContainer.LockWrite();
		const auto findResult = _globalMeterContainer.find(_id);
		if (findResult == _globalMeterContainer.end())
		{
			throw std::logic_error("Who delete the instance in the map?");
		}
		else
		{
			_globalMeterContainer.erase(findResult);
		}
		_lockGlobalMeterContainer.UnlockWrite();
	}
	
	bool IsLimitReached() const
	{
		if (_limit != 0 && _currentFluxCount >= _limit)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void UnsetLimit()
	{
		SetLimit(0);
	}
	
	void SetLimit(uint64_t limit)
	{
		_limit = limit;
	}
	
	static void ShutdownService()
	{
		_systemRunning = false;
		_refreshThread->join();
	}
private:
	static std::unordered_map<uint32_t, std::shared_ptr<flux_meter>> _globalMeterContainer;
	static RWLock _lockGlobalMeterContainer;
	
	static std::shared_ptr<std::thread> _refreshThread;
	static bool _systemRunning;
	
	static flux_meter* _instance;
	static std::atomic<uint32_t> _useCounter;
	
	uint64_t _currentFluxRate;
	uint64_t _limit;
	std::atomic<uint64_t> _currentFluxCount;
	uint32_t _id;
	bool _isThisMeterRunning;
	bool _use;
};
std::unordered_map<uint32_t, std::shared_ptr<flux_meter>> flux_meter::_globalMeterContainer;
RWLock flux_meter::_lockGlobalMeterContainer;
std::shared_ptr<std::thread> flux_meter::_refreshThread;
bool flux_meter::_systemRunning;
flux_meter* flux_meter::_instance = nullptr;
std::atomic<uint32_t> flux_meter::_useCounter = 0;