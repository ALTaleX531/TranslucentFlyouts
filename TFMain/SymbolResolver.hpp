#pragma once
#include "pch.h"

namespace TranslucentFlyouts
{
	class SymbolResolver
	{
	public:
		SymbolResolver(std::wstring_view sessionName);
		~SymbolResolver() noexcept;

		// Pass "*!*" to mask to search all.
		HRESULT Walk(std::wstring_view dllName, std::string_view mask, std::function<bool(PSYMBOL_INFO, ULONG)> callback);
		// Return true if symbol successfully loaded.
		bool IsLoaded();
		// Return true if symbol need to be downloaded.
		bool IsInternetRequired();
	private:
		static BOOL CALLBACK EnumSymbolsCallback(
			PSYMBOL_INFO pSymInfo,
			ULONG SymbolSize,
			PVOID UserContext
		);
		static BOOL CALLBACK SymCallback(
			HANDLE hProcess,
			ULONG ActionCode,
			ULONG64 CallbackData,
			ULONG64 UserContext
		);

		std::wstring_view m_sessionName;
		bool m_printInfo{false};
		bool m_symbolsOK{false};
		bool m_requireInternet{false};
	};
}