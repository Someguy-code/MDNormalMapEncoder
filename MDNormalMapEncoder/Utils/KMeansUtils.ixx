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

	template <class Proj = std::identity>
	static KMeans GetKMeans(const std::vector<T>& _oElements, unsigned int _uGroupsCount, Proj _oProjection = {})
	{
		// Lloyds algorithm: Recompute groups/centroids until convergence (centroids do not change)
		std::vector<T> oCentroids = GetInitialCentroids(_oElements, _uGroupsCount);
		std::vector<std::vector<T>> oGroups;
		float fSumOfSquareDistances;
		float fNewSumOfSquareDistances = 0.f;
		do
		{
			fSumOfSquareDistances = fNewSumOfSquareDistances;
			oGroups = GetGroups(_oElements, oCentroids, fNewSumOfSquareDistances);
			oCentroids = GetCentroids(oGroups, _oProjection);
		} while (fSumOfSquareDistances != fNewSumOfSquareDistances);

		return { fSumOfSquareDistances, std::move(oCentroids), std::move(oGroups) };
	}

private:
	template <class Proj = std::identity>
	static std::vector<T> GetInitialCentroids(const std::vector<T>& _oElements, unsigned int _uGroupsCount, Proj _oProjection = {})
	{
		std::unordered_set<T> oUniqueCentroids;
		oUniqueCentroids.reserve(_uGroupsCount);
		// Forgy method: Take _uGroupsCount random elements (for now, simply the first _uGroupsCount elements)
		for(const T& _oElement : _oElements | std::views::take(_uGroupsCount))
			oUniqueCentroids.insert(std::invoke(_oProjection, _oElement));
		return oUniqueCentroids | std::ranges::to<std::vector>();
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
		for (size_t uCentroidIndex : std::views::iota(static_cast<size_t>(0), _oCentroids.size() - 1))
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

	template <class Proj = std::identity>
	static std::vector<T> GetCentroids(const std::vector<std::vector<T>>& _oGroups, Proj _oProjection = {})
	{
		std::unordered_set<T> oUniqueCentroids;
		oUniqueCentroids.reserve(_oGroups.size());
		for (const std::vector<T>& _oGroup : _oGroups)
		{
			const T oMean = std::accumulate(_oGroup.cbegin(), _oGroup.cend(), T{}) / static_cast<float>(_oGroup.size());
			oUniqueCentroids.insert(std::invoke(_oProjection, oMean));
		}
		return oUniqueCentroids | std::ranges::to<std::vector>();
	}
};