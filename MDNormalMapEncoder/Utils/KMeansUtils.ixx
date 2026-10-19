export module KMeansUtils;

import CollectionUtils;
import MPMCQueue;
import MultithreadUtils;
import std;

export template<HasGetSqrDistance T>
struct KMeansUtils
{
	struct KMeans
	{
		float m_fSumOfSqrDistances;
		std::vector<T> m_oCentroids;
		std::vector<std::vector<T>> m_oGroups;
	};

	template <class CentroidProjectionType = std::identity>
	static KMeans GetKMeans(const std::vector<T>& _oElements, size_t _uGroupsCount, size_t _uMaxInitializationsCount, size_t _uMaxThreads,  CentroidProjectionType _oCentroidProjection = {})
	{
		// Forgy method: Use _uGroupsCount random elements as initial centroids
		const std::vector<T> oElements = [&_oElements]()
		{
			const size_t uSeed = 123;
			std::mt19937 oRandomNumberGenerator{ uSeed };
			std::vector<T> oElements(_oElements);
			std::ranges::shuffle(oElements.begin(), oElements.end(), oRandomNumberGenerator);
			return oElements;
		}();

		const size_t uWholeGroupsCount = std::min(_uGroupsCount * (oElements.size() / _uGroupsCount), _uGroupsCount * _uMaxInitializationsCount) / _uGroupsCount;
		const auto oCentroidsChunks = _oElements | std::views::take(uWholeGroupsCount * _uMaxInitializationsCount) | std::ranges::views::chunk(_uGroupsCount);
		const size_t uThreadsCount = std::min(_uMaxThreads, uWholeGroupsCount);
		return uThreadsCount == 1 ?
			GetKMeansSingleTherad(oElements, oCentroidsChunks, _oCentroidProjection) :
			GetKMeansMultithread(oElements, oCentroidsChunks, uThreadsCount, _oCentroidProjection);
	}

private:

	template <class CentroidProjectionType = std::identity>
	static KMeans GetKMeansSingleTherad(const std::vector<T>& _oElements, std::ranges::input_range auto&& _oCentroidsChunks, CentroidProjectionType _oCentroidProjection = {})
	{
		KMeans oKMeans;
		float fSmallestSumOfSqrDistances = std::numeric_limits<float>::infinity();
		//Compute KMeans with up to _uInitializationsCount initializations and pick the best one
		for (const auto oCentroids : _oCentroidsChunks)
		{
			KMeans oCurrentKMeans = GetKMeansInternal(_oElements, oCentroids | std::ranges::to<std::vector>());
			oCurrentKMeans = CollapseRepeatedCentroids(_oElements, oCurrentKMeans.m_oCentroids, _oCentroidProjection);
			if (fSmallestSumOfSqrDistances > oCurrentKMeans.m_fSumOfSqrDistances)
			{
				fSmallestSumOfSqrDistances = oCurrentKMeans.m_fSumOfSqrDistances;
				oKMeans = std::move(oCurrentKMeans);
			}
		}

		return oKMeans;
	}

	template <class CentroidProjectionType = std::identity>
	static KMeans GetKMeansMultithread(const std::vector<T>& _oElements, std::ranges::input_range auto&& _oCentroidsChunks, size_t _uThreadsCount, CentroidProjectionType _oCentroidProjection = {})
	{
		const size_t uCacheLineSize = std::hardware_destructive_interference_size;
		struct alignas(uCacheLineSize) SharedState
		{
			const std::vector<T>& m_oElements;
			MPMCQueue<size_t, MultithreadUtils::m_uMaxThreadsCount> m_oFreeThreadIndices;
			MPMCQueue<size_t, MultithreadUtils::m_uMaxThreadsCount> m_oFinishedThreadIndices{};
			std::atomic<size_t> m_uBusyThreadsCount{ 0 };

			SharedState(const std::vector<T>& _oElements, size_t _uThreadsCount)
			: m_oElements(_oElements), m_oFreeThreadIndices(std::ranges::views::iota(static_cast<size_t>(0), _uThreadsCount))
			{}

		} oSharedState{ _oElements, _uThreadsCount };

		class alignas(uCacheLineSize) Task
		{
			enum class State
			{
				IDLE,
				READY,
				END
			};
			std::atomic<State> m_eState;
			std::vector<T> m_oCentroids;
			KMeans m_oResult;

			size_t m_uThreadIndex;
			SharedState* m_pSharedState;
			CentroidProjectionType m_oCentroidProjection;

		public:
			Task() = default;

			void Initialize(size_t _uThreadIndex, SharedState* _pSharedState, CentroidProjectionType _oCentroidProjection = {})
			{
				m_uThreadIndex = _uThreadIndex;
				m_pSharedState = _pSharedState;
				m_oCentroidProjection = _oCentroidProjection;
			}

			void Start(std::vector<T> _oCentroids)
			{
				m_oCentroids = std::move(_oCentroids);
				m_pSharedState->m_uBusyThreadsCount.fetch_add(1, std::memory_order_relaxed);
				m_eState.store(State::READY, std::memory_order_release);
				m_eState.notify_one();
			}

			void GetResult(KMeans& _oOutResult)
			{
				_oOutResult = std::move(m_oResult);
				while (!m_pSharedState->m_oFreeThreadIndices.Push(m_uThreadIndex))
				m_eState.store(State::IDLE, std::memory_order_release);
				m_eState.notify_one();
			}

			void Stop()
			{
				m_eState.store(State::END, std::memory_order_release);
				m_eState.notify_one();
			}

			void operator()()
			{
				while(true)
				{
					m_eState.wait(State::IDLE, std::memory_order_acquire);
					if (m_eState.load(std::memory_order_relaxed) == State::END)
						return;
					m_oResult = GetKMeansInternal(m_pSharedState->m_oElements, m_oCentroids);
					m_oResult = CollapseRepeatedCentroids(m_pSharedState->m_oElements, m_oResult.m_oCentroids, m_oCentroidProjection);
					while (!m_pSharedState->m_oFinishedThreadIndices.Push(m_uThreadIndex));
					//Only this therad reads m_eState value, so no need to synchrnoize
					m_eState.store(State::IDLE, std::memory_order_relaxed);
					m_pSharedState->m_uBusyThreadsCount.fetch_sub(1, std::memory_order_relaxed);
					m_pSharedState->m_uBusyThreadsCount.notify_one();
				}
			}
		};

		auto oTasks = std::make_unique<Task[]>(_uThreadsCount);
		std::vector<std::thread> oThreads;
		oThreads.reserve(_uThreadsCount);
		for (size_t uTaskIndex : std::views::iota(static_cast<size_t>(0), _uThreadsCount))
		{
			Task& oTask = oTasks[uTaskIndex];
			oTask.Initialize(uTaskIndex, &oSharedState, _oCentroidProjection);
			oThreads.emplace_back(std::ref(oTask));
		}

		KMeans oKMeans;
		auto fnProcessAllFinishedTasks = [&oSharedState, &oTasks, &oKMeans, fSmallestSumOfSqrDistances = std::numeric_limits<float>::infinity()]() mutable
			{
				size_t uThreadIndex;
				while (oSharedState.m_oFinishedThreadIndices.Pop(uThreadIndex))
				{
					KMeans oCurrentKMeans;
					oTasks[uThreadIndex].GetResult(oCurrentKMeans);
					if (fSmallestSumOfSqrDistances > oCurrentKMeans.m_fSumOfSqrDistances)
					{
						fSmallestSumOfSqrDistances = oCurrentKMeans.m_fSumOfSqrDistances;
						oKMeans = std::move(oCurrentKMeans);
					}
				}
			};

		//Compute KMeans with up to _uInitializationsCount initializations and pick the best one
		for (const auto oCentroids : _oCentroidsChunks)
		{
			//Wait while all threads are busy
			oSharedState.m_uBusyThreadsCount.wait(_uThreadsCount, std::memory_order_relaxed);

			size_t uThreadIndex;
			while (!oSharedState.m_oFreeThreadIndices.Pop(uThreadIndex))
				fnProcessAllFinishedTasks();

			oTasks[uThreadIndex].Start(oCentroids | std::ranges::to<std::vector>());
		}

		while (oSharedState.m_uBusyThreadsCount.load(std::memory_order_relaxed) > 0)
			fnProcessAllFinishedTasks();

		for (size_t uTaskIndex : std::views::iota(static_cast<size_t>(0), _uThreadsCount))
		{
			oTasks[uTaskIndex].Stop();
			oThreads[uTaskIndex].join();
		}	

		return oKMeans;
	}

	static KMeans GetKMeansInternal(const std::vector<T>& _oElements, std::vector<T> _oCentroids)
	{
		// Lloyds algorithm: Recompute groups/centroids until convergence (centroids do not change)
		std::vector<std::vector<T>> oGroups;
		float fSumOfSquareDistances;
		float fNewSumOfSquareDistances = 0.f;
		do
		{
			fSumOfSquareDistances = fNewSumOfSquareDistances;
			oGroups = GetGroups(_oElements, _oCentroids, fNewSumOfSquareDistances);
			_oCentroids = GetCentroids(oGroups);
		} while (fSumOfSquareDistances != fNewSumOfSquareDistances);

		return { fSumOfSquareDistances, std::move(_oCentroids), std::move(oGroups) };
	}

	static std::vector<std::vector<T>> GetGroups(const std::vector<T>& _oElements, const std::vector<T>& _oCentroids, float& _fSumOfSquareDistances)
	{
		_fSumOfSquareDistances = 0.f;
		auto oGroups = std::vector<std::vector<T>>(_oCentroids.size());
		std::for_each(oGroups.begin(), oGroups.end(),
			[uElementsCount = _oElements.size()](auto& _oGroup) {_oGroup.reserve(uElementsCount);});
		for (const T& oElement : _oElements)
		{
			const auto [uClosestCentroidIndex, uClosestSqrDistance] = GetClosestCentroidIndexAndSqrDistance(oElement, _oCentroids);
			oGroups[uClosestCentroidIndex].emplace_back(oElement);
			_fSumOfSquareDistances += uClosestSqrDistance;
		}
		return oGroups;
	}

	static std::tuple<size_t, float> GetClosestCentroidIndexAndSqrDistance(const T& _oElement, const std::vector<T>& _oCentroids)
	{
		size_t uClosestIndex = 0;
		float fClosestSqrDistance = std::numeric_limits<float>::infinity();
		for (size_t uCentroidIndex : std::views::iota(static_cast<size_t>(0), _oCentroids.size()))
		{
			const T& oCentroid = _oCentroids[uCentroidIndex];
			const float fSqrDistance = _oElement.GetSqrDistance(oCentroid);
			if (fSqrDistance < fClosestSqrDistance)
			{
				uClosestIndex = uCentroidIndex;
				fClosestSqrDistance = fSqrDistance;
			}
		}
		return { uClosestIndex , fClosestSqrDistance };
	}

	static std::vector<T> GetCentroids(const std::vector<std::vector<T>>& _oGroups)
	{
		std::vector<T> oCentroids;
		oCentroids.reserve(_oGroups.size());
		for (const std::vector<T>& _oGroup : _oGroups)
			oCentroids.push_back(std::accumulate(_oGroup.cbegin(), _oGroup.cend(), T{}) / static_cast<float>(_oGroup.size()));
		return oCentroids;
	}

	template <class CentroidProjectionType = std::identity>
	static KMeans CollapseRepeatedCentroids(const std::vector<T>& _oElements, const std::vector<T>& _oCentroids, CentroidProjectionType _oCentroidProjection = {})
	{
		size_t uCentroidsCount = _oCentroids.size();

		// Project centroids if needed
		std::vector<T> oCollaspedCentroids;
		oCollaspedCentroids.reserve(uCentroidsCount);
		std::transform(_oCentroids.cbegin(), _oCentroids.cend(), std::back_insert_iterator(oCollaspedCentroids), _oCentroidProjection);

		// Remove repeated centroids
		std::sort(oCollaspedCentroids.begin(), oCollaspedCentroids.end());
		auto itLast = std::unique(oCollaspedCentroids.begin(), oCollaspedCentroids.end());
		oCollaspedCentroids.erase(itLast, oCollaspedCentroids.end());

		// Re-group according to the new centroids
		float fSumOfSquareDistances = 0;
		std::vector<std::vector<T>> oGroups = GetGroups(_oElements, oCollaspedCentroids, fSumOfSquareDistances);

		// Remove any empty groups
		auto oCentroidsAndGroups = std::views::zip(oCollaspedCentroids, oGroups);
		auto [itLastCentroidAndGroups, _] = std::ranges::remove_if(oCentroidsAndGroups, [](const auto& _oTuple) {
			const auto& [_, oGroup] = _oTuple;
			return oGroup.size() == 0;
		});
		size_t uDistance = std::distance(itLastCentroidAndGroups, oCentroidsAndGroups.end());
		if (uDistance > 0)
		{
			oCollaspedCentroids.erase(oCollaspedCentroids.end() - uDistance, oCollaspedCentroids.end());
			oGroups.erase(oGroups.end() - uDistance, oGroups.end());
		}

		return { fSumOfSquareDistances, std::move(oCollaspedCentroids), std::move(oGroups) };
	}
};