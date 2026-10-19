module;
#include <cassert>

export module MPMCQueue;

import std;

//Bounded lock-free multi-producer multi-consumer queue. Based on Sagar's code (https://towardsdev.com/lock-free-programming-in-c-compare-and-swap-without-the-magic-4e8a8f278d90)
export template<typename T, size_t N>
class MPMCQueue
{
	class alignas(std::hardware_destructive_interference_size) Node
	{
		std::atomic<size_t> m_uSequenceNumber;
		alignas(T) unsigned char m_oValue[sizeof(T)];

		T* GetValue()
		{
			return std::launder(reinterpret_cast<T*>(m_oValue));
		}

		template<bool bNeedsSynchronizartion = true>
		void SetQueued(size_t _uSequenceNumber)
		{
			m_uSequenceNumber.store(_uSequenceNumber + 1, bNeedsSynchronizartion ? std::memory_order_release : std::memory_order_relaxed);
		}

		void SetDequeued(size_t _uSequenceNumber)
		{
			m_uSequenceNumber.store(_uSequenceNumber + N, std::memory_order_release);
		}

	public:
		void Initialize(size_t _uSequenceNumber)
		{
			m_uSequenceNumber.store(_uSequenceNumber, std::memory_order_relaxed);
		}

		template<typename... Arguments>
		void Initialize(size_t _uSequenceNumber, Arguments&&... _oArguments) noexcept
		{
			new (GetValue()) T(std::forward<Arguments>(_oArguments)...);
			SetQueued<false>(_uSequenceNumber);
		}

		size_t GetSequenceNumber(std::memory_order _oMermoryOrder) const
		{
			return m_uSequenceNumber.load(_oMermoryOrder);
		}

		template<typename... Arguments>
		void Emplace(size_t _uTailIndex, Arguments&&... _oArguments) noexcept
		{
			new (GetValue()) T(std::forward<Arguments>(_oArguments)...);
			SetQueued(_uTailIndex);
		}

		void Pop(size_t _uHeadIndex, T& _oValue)
		{
			T* pValue = GetValue();
			_oValue = std::move(*pValue);
			DestroyValue();
			SetDequeued(_uHeadIndex);
		}

		void DestroyValue()
		{
			T* pValue = GetValue();
			pValue->~T();
		}
	};

	Node m_oNodesRing[N];

	std::atomic<size_t> m_uTailIndex{ 0 };
	std::atomic<size_t> m_uHeadIndex{ 0 };

	constexpr bool IsQueueEmpty(size_t _uHeadIndex, size_t _uNodeSequenceIndex)
	{
		return _uNodeSequenceIndex < _uHeadIndex + 1;
	}

	constexpr bool IsQueueFull(size_t _uTailIndex, size_t _uNodeSequenceIndex)
	{
		return _uNodeSequenceIndex < _uTailIndex;
	}

	constexpr bool IsNodeQueued(size_t _uNodeIndex, size_t _uNodeSequenceIndex)
	{
		return _uNodeSequenceIndex == _uNodeIndex + 1;
	}

	constexpr bool IsNodeDequeued(size_t _uNodeIndex, size_t _uNodeSequenceIndex)
	{
		return _uNodeSequenceIndex > _uNodeIndex + 1;
	}

	Node& GetNode(size_t _uNodeIndex)
	{
		return m_oNodesRing[_uNodeIndex % N];
	}

	bool TryAcquireNode(size_t _uTailndex)
	{
		return m_uTailIndex.compare_exchange_weak(_uTailndex, _uTailndex + 1,
			std::memory_order_release, std::memory_order_relaxed);
	}

	bool TryRelaseNode(size_t _uHeadIndex)
	{
		return m_uHeadIndex.compare_exchange_weak(_uHeadIndex, _uHeadIndex + 1,
			std::memory_order_release, std::memory_order_relaxed);
	}

public:
	MPMCQueue()
	{
		for(size_t uNodeIndex: std::views::iota(static_cast<size_t>(0), N))
			m_oNodesRing[uNodeIndex].Initialize(uNodeIndex);
	}

	//Initialize the queue with a range of initial values. Will throw if the range has more than N elements.
	MPMCQueue(std::ranges::input_range auto&& _oInitialValuesRange)
		: MPMCQueue()
	{
		size_t uNodeIndex = 0;
		for (T&& oElement : _oInitialValuesRange)
		{
			assert(uNodeIndex < N);
			//m_oNodesRing[uNodeIndex++].Initialize(uNodeIndex, std::forward<T>(oElement));
			(void)Emplace(std::forward<T>(oElement));
			++uNodeIndex;
		}
		//m_uTailIndex.store(uNodeIndex);
	}

	~MPMCQueue()
	{
		for (size_t uNodeIndex : std::views::iota(
			m_uHeadIndex.load(std::memory_order_relaxed),
			m_uTailIndex.load(std::memory_order_relaxed) + 1))
		{
			Node& oNode = GetNode(uNodeIndex);
			if (IsNodeQueued(uNodeIndex, oNode.GetSequenceNumber(std::memory_order_relaxed)))
				oNode.DestroyValue();
		}
	}

	MPMCQueue(const MPMCQueue&) = delete;
	MPMCQueue& operator=(const MPMCQueue&) = delete;

	template<typename... Arguments>
	[[nodiscard]] bool Emplace(Arguments&&... _oArguments) noexcept
	{
		static_assert(std::is_constructible_v<T, Arguments...>);

		size_t uTailIndex = m_uTailIndex.load(std::memory_order_relaxed);
		std::optional<bool> bSuccess;
		do
		{
			Node& oNode = GetNode(uTailIndex);
			size_t uNodeSequence = oNode.GetSequenceNumber(std::memory_order_acquire);
			if (IsQueueFull(uTailIndex, uNodeSequence))
				bSuccess = false;
			else if (IsNodeQueued(uTailIndex, uNodeSequence))
				uTailIndex = m_uTailIndex.load(std::memory_order_relaxed);
			else if (TryAcquireNode(uTailIndex))
			{
				oNode.Emplace(uTailIndex, std::forward<Arguments>(_oArguments)...);
				bSuccess = true;
			}
		} while (!bSuccess.has_value());
		return *bSuccess;
	}

	template <typename U>
	[[nodiscard]] bool Push(U&& _oValue) noexcept
	{
		return Emplace(std::forward<U>(_oValue));
	}

	[[nodiscard]] bool Pop(T& _oOutValue) noexcept
	{
		static_assert(std::is_nothrow_move_assignable_v<T>);

		size_t uHeadIndex = m_uHeadIndex.load(std::memory_order_relaxed);
		std::optional<bool> bSuccess;
		do
		{
			Node& oNode = GetNode(uHeadIndex);
			size_t uNodeSequence = oNode.GetSequenceNumber(std::memory_order_acquire);
			if (IsQueueEmpty(uHeadIndex, uNodeSequence))
				bSuccess = false;
			else if (IsNodeDequeued(uHeadIndex, uNodeSequence))
				uHeadIndex = m_uHeadIndex.load(std::memory_order_relaxed);
			else if (TryRelaseNode(uHeadIndex))
			{
				oNode.Pop(uHeadIndex, _oOutValue);
				bSuccess = true;
			}
		} while (!bSuccess.has_value());
		return *bSuccess;
	}
};