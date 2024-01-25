#pragma once
#include "HookHelper.hpp"

namespace TranslucentFlyouts::HookHelper
{
	template <size_t hookCount, size_t possibleImportCount>
	using HookDispatcherDependency = std::array<std::tuple<std::array<std::string_view, possibleImportCount>, LPCSTR, PVOID>, hookCount>;

	template <size_t hookCount, size_t possibleImportCount>
	struct HookDispatcher
	{
		const HookDispatcherDependency<hookCount, possibleImportCount>& hookDependency{};
		std::array<std::pair<OffsetStorage, std::optional<OffsetStorage>>, hookCount> hookInfoCache{};
		std::array<std::pair<PVOID, std::optional<HMODULE>>, hookCount> hookTable{};
		std::array<size_t, hookCount> hookRef{};
		PVOID moduleAddress{};

		void CacheHookData()
		{
			if (hookInfoCache[0].first.IsValid())
			{
				return;
			}
			auto dllPath{ wil::GetModuleFileNameW<std::wstring, MAX_PATH + 1>(reinterpret_cast<HMODULE>(moduleAddress)) };
			if (dllPath.empty())
			{
				return;
			}

			PVOID* functionAddress{ nullptr };
			HMODULE* moduleHandle{ nullptr };
			auto imageMapper{ ImageMapper(dllPath) };
			auto initialize_offset_storage = [&](size_t index)
			{
				hookInfoCache[index] = std::make_pair(
					OffsetStorage::From(imageMapper.GetBaseAddress(), functionAddress),
					!moduleHandle ? std::nullopt : std::make_optional(OffsetStorage::From(imageMapper.GetBaseAddress(), moduleHandle))
				);

#ifdef _DEBUG
				OutputDebugStringW(
					std::format(
						L"[{}] index: {}, offset stored: {}, {}\n",
						dllPath,
						index,
						hookInfoCache[index].first.value,
						hookInfoCache[index].second.has_value() ? std::format(L"{}", hookInfoCache[index].second.value().value) : L"empty"
					).c_str()
				);
#endif
				functionAddress = nullptr;
				moduleHandle = nullptr;
			};
			auto bind = [&](const std::pair<HMODULE*, PVOID*> value)
			{
				functionAddress = value.second;
				moduleHandle = value.first;

				return functionAddress != nullptr;
			};

			for (size_t i = 0; i < hookDependency.size(); i++)
			{
				auto [possibleImport, functionName, detourFunction]{hookDependency[i]};
				for (auto importDll : possibleImport)
				{
					if (functionAddress = GetIAT(imageMapper.GetBaseAddress(), importDll, functionName); functionAddress)
					{
						OutputDebugStringA(
							std::format(
								"{} - {} offset cached[IAT]\n",
								importDll,
								functionName
							).c_str()
						);
						initialize_offset_storage(i);
						break;
					}
					if (bind(GetDelayloadIAT(imageMapper.GetBaseAddress(), importDll, functionName)))
					{
						OutputDebugStringA(
							std::format(
								"{} - {} offset cached [DelayloadIAT]\n",
								importDll,
								functionName
							).c_str()
						);
						initialize_offset_storage(i);
						break;
					}
				}
			}
		}
		bool IsHookEnabled(size_t index) const { return hookRef[index] > 0; }
		bool EnableHook(size_t index, bool enable)
		{
			if (!moduleAddress)
			{
				return false;
			}

			hookRef[index] += enable ? 1 : -1;
			bool hookChanged{ false };
			if ((enable && hookRef[index] == 1) || (!enable && hookRef[index] == 0))
			{
				hookChanged = true;
			}

			if (!hookInfoCache[index].first.IsValid())
			{
				CacheHookData();
			}

			auto& [functionAddressOffset, moduleHandleOffset] {hookInfoCache[index]};
			if (enable)
			{
				if (moduleHandleOffset.has_value())
				{
					HookHelper::ResolveDelayloadIAT(
						std::make_pair(
							reinterpret_cast<HMODULE*>(moduleHandleOffset.value().To(moduleAddress)),
							reinterpret_cast<PVOID*>(functionAddressOffset.To(moduleAddress))
						),
						moduleAddress,
						std::get<0>(std::get<0>(hookDependency[index])),
						std::get<1>(hookDependency[index])
					);
				}
				hookTable[index] = std::make_pair(
					*reinterpret_cast<PVOID*>(functionAddressOffset.To(moduleAddress)),
					moduleHandleOffset.has_value() ? 
					std::make_optional(*reinterpret_cast<HMODULE*>(moduleHandleOffset.value().To(moduleAddress))) :
					std::nullopt
				);

				HookHelper::WriteMemory(functionAddressOffset.To(moduleAddress), [&]
				{
					*reinterpret_cast<PVOID*>(functionAddressOffset.To(moduleAddress)) = std::get<2>(hookDependency[index]);
				});
			}
			else
			{
				HookHelper::WriteMemory(functionAddressOffset.To(moduleAddress), [&]
				{
					*reinterpret_cast<PVOID*>(functionAddressOffset.To(moduleAddress)) = hookTable[index].first;
					hookTable[index].first = nullptr;
				});
				if (moduleHandleOffset.has_value())
				{
					HookHelper::WriteMemory(moduleHandleOffset.value().To(moduleAddress), [&]
					{
						*reinterpret_cast<PVOID*>(moduleHandleOffset.value().To(moduleAddress)) = hookTable[index].second.value();
						hookTable[index].second = std::nullopt;
					});
				}
			}

			return true;
		}
		bool EnableHookNoRef(size_t index, bool enable)
		{
			if ((IsHookEnabled(index) && enable) || (!IsHookEnabled(index) && !enable))
			{
				return false;
			}

			return EnableHook(index, enable);
		}
		void DisableAllHooks()
		{
			if (!moduleAddress)
			{
				return;
			}

			for (size_t i = 0; i < hookInfoCache.size(); i++)
			{
				auto& [functionAddressOffset, moduleHandleOffset] {hookInfoCache[i]};

				if (hookTable[i].first)
				{
					HookHelper::WriteMemory(functionAddressOffset.To(moduleAddress), [&]
					{
						*reinterpret_cast<PVOID*>(functionAddressOffset.To(moduleAddress)) = hookTable[i].first;
						hookTable[i].first = nullptr;
					});
				}
				if (moduleHandleOffset.has_value())
				{
					HookHelper::WriteMemory(moduleHandleOffset.value().To(moduleAddress), [&]
					{
						*reinterpret_cast<PVOID*>(moduleHandleOffset.value().To(moduleAddress)) = hookTable[i].second.value();
						hookTable[i].second = std::nullopt;
					});
				}
				hookRef[i] = 0;
			}
		}
		template <typename T, size_t index>
		__forceinline auto GetOrg() const { return reinterpret_cast<T>(hookTable[index].first); }
		template <size_t index>
		__forceinline auto GetOrg() const { return reinterpret_cast<PVOID>(hookTable[index].first); }
		HookDispatcher(
			const HookDispatcherDependency<hookCount, possibleImportCount>& dependency,
			PVOID targetDll = nullptr
		) : hookDependency{ dependency }, moduleAddress{ targetDll }
		{
		}
		HookDispatcher() = delete;
	};

	template <size_t hookCount, size_t possibleImportCount>
	HookDispatcher(
		const HookDispatcherDependency<hookCount, possibleImportCount>&,
		PVOID
	) -> HookDispatcher<hookCount, possibleImportCount>;
}