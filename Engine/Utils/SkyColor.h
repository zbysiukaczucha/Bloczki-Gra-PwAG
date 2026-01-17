#pragma once
#include <DirectXMath.h>

namespace Utils
{
	struct SkyColors
	{
		DirectX::XMFLOAT4 zenithColor;
		DirectX::XMFLOAT4 horizonColor;
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

	[[nodiscard]] inline SkyColors CalculateSkyColors(float timeOfDay)
	{
		using namespace DirectX;
		XMVECTOR dayZenith	= XMVectorSet(0.2f, 0.5f, 1.0f, 1.0f); // Deep Blue
		XMVECTOR dayHorizon = XMVectorSet(0.6f, 0.8f, 1.0f, 1.0f); // Light Blue

		XMVECTOR sunsetZenith  = XMVectorSet(0.2f, 0.2f, 0.4f, 1.0f); // Purple-ish
		XMVECTOR sunsetHorizon = XMVectorSet(0.8f, 0.4f, 0.1f, 1.0f); // Orange

		XMVECTOR nightZenith  = XMVectorSet(0.0f, 0.0f, 0.1f * 0.05f, 1.0f); // Dark Blue/Black
		XMVECTOR nightHorizon = XMVectorSet(0.0f, 0.0f, 0.2f * 0.05f, 1.0f); // Dark Horizon

		XMVECTOR currentZenith, currentHorizon;

		// reverse time of day if after noon, the gradient works the same but in reverse for that case
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
			float rawT	   = GetTimePercent(TIME_NIGHT_END, TIME_SUNRISE_PEAK, timeOfDay);
			float easeT	   = SmoothStep(rawT);
			currentZenith  = XMVectorLerp(nightZenith, sunsetZenith, easeT);
			currentHorizon = XMVectorLerp(nightHorizon, sunsetHorizon, easeT);
		}
		else // moving towards noon
		{
			float rawT	   = GetTimePercent(TIME_SUNRISE_PEAK, TIME_NOON_PEAK, timeOfDay);
			float easeT	   = SmoothStep(rawT);
			currentZenith  = XMVectorLerp(sunsetZenith, dayZenith, easeT);
			currentHorizon = XMVectorLerp(sunsetHorizon, dayHorizon, easeT);
		}


		SkyColors result;
		XMStoreFloat4(&result.zenithColor, currentZenith);
		XMStoreFloat4(&result.horizonColor, currentHorizon);

		return result;
	}
} // namespace Utils
