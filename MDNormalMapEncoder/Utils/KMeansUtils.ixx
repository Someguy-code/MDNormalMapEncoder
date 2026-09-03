export module KMeansUtils;

import CollectionUtils;
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
	static KMeans GetKMeans(const std::vector<T>& _oElements, unsigned int _uGroupsCount, unsigned int _uInitializationsCount, CentroidProjectionType _oCentroidProjection = {})
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

		KMeans oKMeans;
		float fSmallestSumOfSqrDistances = std::numeric_limits<float>::infinity();
		//Compute KMeans with up to _uInitializationsCount initializations and pick the best one
		for (const auto oCentroids : oElements | std::views::take(_uGroupsCount * _uInitializationsCount) | std::ranges::views::chunk(_uGroupsCount))
		{
			// If the chunk has not enough elements, just end
			if (oCentroids.size() < _uGroupsCount)
				break;
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

private:

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
		for (const T& oElement : _oElements)
		{
			const auto [uClosestCentroidIndex, uClosestSqrDistance] = GetClosestCentroidIndexAndSqrDistance(oElement, _oCentroids);
			oGroups[uClosestCentroidIndex].push_back(oElement);
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