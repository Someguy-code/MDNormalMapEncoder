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

	static KMeans GetKMeans(const std::vector<T>& _oElements, unsigned int _uGroupsCount, unsigned int _uInitializationsCount)
	{
		// Forgy method: Take _uGroupsCount random elements
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
			const KMeans oCurrentKMeans = GetKMeansInternal(_oElements, oCentroids | std::ranges::to<std::vector>());
			if (fSmallestSumOfSqrDistances > oCurrentKMeans.m_fSumOfSqrDistances)
			{
				fSmallestSumOfSqrDistances = oCurrentKMeans.m_fSumOfSqrDistances;
				oKMeans = std::move(oCurrentKMeans);
			}
		}

		return oKMeans;
	}

private:
	/*static std::vector<T> GetInitialCentroids(std::vector<T> _oElements, unsigned int _uGroupsCount)
	{
		// Forgy method: Take _uGroupsCount random elements (for now, simply the first _uGroupsCount elements)
		return _oElements | std::views::take(_uGroupsCount) | std::ranges::to<std::vector>();
	}*/

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

	template <class Proj = std::identity>
	static std::vector<T> GetCentroids(const std::vector<std::vector<T>>& _oGroups)
	{
		std::vector<T> oCentroids;
		oCentroids.reserve(_oGroups.size());
		for (const std::vector<T>& _oGroup : _oGroups)
			oCentroids.push_back(std::accumulate(_oGroup.cbegin(), _oGroup.cend(), T{}) / static_cast<float>(_oGroup.size()));
		return oCentroids;
	}
};