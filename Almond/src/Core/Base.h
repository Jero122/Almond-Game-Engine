#pragma once
#include <memory>

#ifdef ALM_PLATFORM_WINDOWS
	#ifdef ALM_BUILD_DLL
		#define ALMOND_API __declspec(dllexport)
	#else
		#define ALMOND_API __declspec(dllimport)
	#endif
#else
	#error Almond only suppors windows!
#endif


template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}