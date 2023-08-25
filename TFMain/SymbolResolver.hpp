#pragma once
#include "pch.h"
#include "TFMain.hpp"

namespace TranslucentFlyouts
{
	class SymbolResolver
	{
	public:
		SymbolResolver(std::wstring_view sessionName, const TFMain::InteractiveIO& io);
		~SymbolResolver() noexcept;

		// Pass "*!*" to mask to search all.
		HRESULT Walk(std::wstring_view dllName, std::string_view mask, std::function<bool(PSYMBOL_INFO, ULONG)> callback);
		// Return true if symbol successfully loaded.
		bool GetSymbolStatus();
		// Return true if symbol need to be downloaded.
		bool GetSymbolSource();
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
		const TFMain::InteractiveIO& m_io;
		bool m_printInfo{false};
		bool m_symbolsOK{false};
		bool m_requireInternet{false};
	};
}