#pragma once
#include <DirectXMath.h>
#include <algorithm>

namespace Utils
{
	struct SunColor
	{
		DirectX::XMFLOAT3 sunlightColor;
		DirectX::XMFLOAT4 sunlightIntensity;
	};

	template <typename T>
	[[nodiscard]] T GetTimePercent(T minTime, T maxTime, T currentTime)
	{
		T t	  = (currentTime - minTime) / (maxTime - minTime);
		T min = 0.0;
		T max = 1.0;
		return std::clamp(t, min, max);
	}

	template <typename T>
	[[nodiscard]] T SmoothStep(T t)
	{
		return t * t * (3.0 - 2.0 * t);
	}

	[[nodiscard]] inline SunColor CalculateSunColors(float timeOfDay)
	{
		using namespace DirectX;

		// reverse time of day if after noon, the gradient works the same but in reverse for that case
		constexpr XMFLOAT3 sunNoonColor;
		if (timeOfDay >= 0.5f)
		{
			timeOfDay = 1.0f - timeOfDay;
		}

		static constexpr float TIME_NIGHT_END	 = 0.1f;
		static constexpr float TIME_SUNRISE_PEAK = 0.25f;
		static constexpr float TIME_NOON_PEAK	 = 0.5f;

		// 0 and 1 - midnight, 0.5 noon
		timeOfDay = std::clamp(timeOfDay, 0.0f, 1.0f);
		if (timeOfDay <= TIME_SUNRISE_PEAK) // moving towards sunrise
		{
			float t = timeOfDay / 0.25f;
			// t			   = std::clamp(t - 0.2f, 0.0f, 1.0f);
			t = SmoothStep(t);

			float rawT	= GetTimePercent(TIME_NIGHT_END, TIME_SUNRISE_PEAK, timeOfDay);
			float easeT = SmoothStep(rawT);
		}
		else // moving towards noon
		{
			// timeOfDay starts at value > 0.25f
			float t = (timeOfDay - 0.25f) / 0.25f;
			t		= SmoothStep(t);
			// t			   = std::clamp(t - 0.2f, 0.0f, 1.0f);

			float rawT	= GetTimePercent(TIME_SUNRISE_PEAK, TIME_NOON_PEAK, timeOfDay);
			float easeT = SmoothStep(rawT);
		}


		SunColor result;

		return result;
	}
} // namespace Utils
