#include "pch.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "Api.hpp"
#include "HookHelper.hpp"
#include "DiagnosticsHooks.hpp"
#include "DiagnosticsHandler.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::DiagnosticsHooks
{
	using namespace std::literals;
	struct CDependencyObject;
	struct CDependencyProperty;
	struct DirectUI_DXamlCore;
	struct CValue;
	struct PropertyChangedParams;
	struct DeferredElementStateChange;
	enum class KnownTypeIndex;
	enum class KnownPropertyIndex;
	enum class DirectUI_DXamlCore_GetPeerPrivateCreateMode
	{
		GetOnly,
		CreateIfNecessary
	};

	namespace WUX
	{
		struct DirectUI_DependencyObject : winrt::impl::abi_t<winrt::Windows::UI::Xaml::IDependencyObject>
		{
			virtual HRESULT STDMETHODCALLTYPE GetValue(
				const CDependencyProperty* pDependecyProperty,
				IInspectable** ppValue
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE PrepareState() = 0;
			virtual bool STDMETHODCALLTYPE IsExternalObjectReference() = 0;
			virtual HRESULT STDMETHODCALLTYPE OnParentUpdated(
				CDependencyObject* pOldParentCore,
				CDependencyObject* pNewParentCore,
				bool isNewParentAlive
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnTreeParentUpdated(CDependencyObject* pNewParent, bool isParentAlive) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnCollectionChanged(unsigned int nCollectionChangeType, unsigned int nIndex) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnInheritanceContextChanged() = 0;
			virtual HRESULT STDMETHODCALLTYPE DisconnectFrameworkPeerCore() = 0;
			virtual HRESULT STDMETHODCALLTYPE GetDefaultValue2(const CDependencyProperty* pDependencyObject, CValue* pValue) = 0;
			virtual HRESULT STDMETHODCALLTYPE EnterImpl(
				BOOL bLive,
				BOOL bSkipNameRegistration,
				BOOL bCoercedIsEnabled
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE LeaveImpl(
				BOOL bLive,
				BOOL bSkipNameRegistration,
				BOOL bCoercedIsEnabled
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnPropertyChanged2(PropertyChangedParams& args) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnChildUpdated(DirectUI_DependencyObject* pChild) = 0;
			virtual HRESULT STDMETHODCALLTYPE NotifyDeferredElementStateChanged(
				KnownPropertyIndex propertyIndex,
				DeferredElementStateChange state,
				unsigned int collectionIndex
			) = 0;
		};

		namespace MyDirectUI_DependencyObject
		{
			HRESULT STDMETHODCALLTYPE EnterImpl(
				DirectUI_DependencyObject* This,
				BOOL bLive,
				BOOL bSkipNameRegistration,
				BOOL bCoercedIsEnabled
			);
			HRESULT STDMETHODCALLTYPE LeaveImpl(
				DirectUI_DependencyObject* This,
				BOOL bLive,
				BOOL bSkipNameRegistration,
				BOOL bCoercedIsEnabled
			);

			void HookAttach(DirectUI_DependencyObject* This);
			void HookDetach(DirectUI_DependencyObject* This);
			void DetachAllHooks();

			using DirectUI_DependencyObjectOrgs = std::tuple<
				size_t,
				decltype(&EnterImpl),
				decltype(&LeaveImpl)
			>;
			std::unordered_map<
				void**,
				DirectUI_DependencyObjectOrgs
			> g_hookedObject{};
		}

		HRESULT WINAPI MyDirectUI_DXamlCore_GetPeerPrivate(
			DirectUI_DXamlCore* This,
			IInspectable* pOuter,
			CDependencyObject* pDependencyObject,
			DirectUI_DXamlCore_GetPeerPrivateCreateMode createMode,
			KnownTypeIndex hClass,
			BOOL bInternalRef,
			BOOL fCreatePegged,
			BOOL* pIsPendingDelete,
			DirectUI_DependencyObject** ppObject
		);
		UCHAR g_DirectUI_DXamlCore_TryGetPeer_AsmCode[]
		{
			0x48, 0x89, 0x5C, 0x24, 0x08,	// mov     [rsp + arg_0], rbx
			0x44, 0x89, 0x4C, 0x24, 0x20,	// mov     [rsp + arg_18], createMode
			0x48, 0x89, 0x54, 0x24, 0x10,	// mov     [rsp + arg_8], pOuter
			0x55,							// push    rbp
			0x56,							// push    rsi
			0x57,							// push    rdi
			0x41, 0x54,						// push    r12
			0x41, 0x55,						// push    r13
			0x41, 0x56,						// push    r14
			0x41, 0x57,						// push    r15
			0x48, 0x83, 0xEC, 0XFF,			// sub     rsp, xxh
			0x33, 0xED,						// xor	   ebp, ebp
			0x41, 0x8B, 0xC1,				// mov     eax, createMode
			0XFF, 0x8B, 0XFF,				// mov     rsi, pCoreDO
			0xFF, 0x8B, 0XFF,				// mov     r14, this
		};
		UCHAR g_DirectUI_DXamlCore_TryGetPeer_AsmCode_Win10[]
		{
			0x48, 0x8B, 0xC4,					// mov     rax, rsp
			0x48, 0x89, 0x58, 0x08,				// mov     [rax + 8], rbx
			0x44, 0x89, 0x48, 0x20,				// mov     [rax + 20h], createMode
			0x48, 0x89, 0x50, 0x10,				// mov     [rax + 10h], pOuter
			0x55,								// push    rbp
			0x56,								// push    rsi
			0x57,								// push    rdi
			0x41, 0x54,							// push    r12
			0x41, 0x55,							// push    r13
			0x41, 0x56,							// push    r14
			0x41, 0x57,							// push    r15
			0x48, 0x83, 0xEC, 0X50,				// sub     rsp, 50h
			0x33, 0xED,							// xor	   ebp, ebp
			0x4D, 0x8B, 0xF8,					// mov     r15, pCoreDO
			0x8B, 0xDD,							// mov     ebx, ebp
			0x48, 0x8B, 0xF9,					// mov     rdi, this
			0x48, 0x89, 0x58, 0x18,				// mov     [rax+18h], rbx
			0x44, 0x8B, 0xE5,					// mov     r12d, ebp
			0x4D, 0x85, 0xC0,					// test    pCoreDO, pCoreDO
			0x0F, 0x84, 0XFF, 0XFF, 0XFF, 0XFF	// jz      xxxx
		};


		UCHAR g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode[]
		{
			0x48, 0x89, 0x5C, 0x24, 0x18,				// mov     [rsp-28h+arg_10], rbx
			0x48, 0x89, 0x54, 0x24, 0x10,				// mov     [rsp-28h+queue], rdx
			0x48, 0x89, 0x4C, 0x24, 0x08,				// mov     [rsp-28h+debugSettings.ptr_], rcx
			0x55,										// push    rbp
			0x56,										// push    rsi
			0x57,										// push    rdi
			0x41, 0x56,									// push    r14
			0x41, 0x57,									// push    r15
			0x48, 0x8B, 0xEC,							// mov     rbp, rsp
			0x48, 0x83, 0xEC, 0x30,						// sub     rsp, 30h
			0x45, 0x8B, 0xF8,							// mov     r15d, threadId
			0x4C, 0x8B, 0xF2,							// mov     r14, rdx
			0x33, 0xF6,									// xor     esi, esi
			0x89, 0x75, 0xF0,							// mov     [rbp+var_10], esi
			0x48, 0x89, 0x32,							// mov     [rdx], rsi
			0xC7, 0x45, 0xF0, 0x01, 0x00, 0x00, 0x00,	// mov     [rbp+var_10], 1
			0xE8, 0XFF, 0XFF, 0XFF, 0XFF,				// call    ?GetCurrentNoRef@FrameworkApplication@DirectUI@@SAPEAV12@XZ ; DirectUI::FrameworkApplication::GetCurrentNoRef(void)
			0x48, 0x85, 0xC0,							// test    rax, rax
		};

		constexpr size_t DirectUI_DependencyObject_EnterImpl_VTableIndex{ 21 };
#pragma data_seg(".shared")
		HookHelper::OffsetStorage g_DirectUI_DXamlCore_GetPeerPrivate_Offset{ 0 };
		HookHelper::OffsetStorage g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset{ 0 };
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")
		bool g_hookEnabled{false};
		decltype(&MyDirectUI_DXamlCore_GetPeerPrivate) g_actualDirectUI_DXamlCore_GetPeerPrivate{ nullptr };

		void EnableGetPeerPrivateHook(bool enable)
		{
			if (g_actualDirectUI_DXamlCore_GetPeerPrivate)
			{
				HookHelper::Detours::Write([&]()
				{
					if (enable)
					{
						HookHelper::Detours::Attach(reinterpret_cast<PVOID*>(&g_actualDirectUI_DXamlCore_GetPeerPrivate), MyDirectUI_DXamlCore_GetPeerPrivate);
					}
					else
					{
						HookHelper::Detours::Detach(reinterpret_cast<PVOID*>(&g_actualDirectUI_DXamlCore_GetPeerPrivate), MyDirectUI_DXamlCore_GetPeerPrivate);
					}
				});

				g_hookEnabled = enable;
			}
		}
	}

	namespace MUX
	{
		struct DirectUI_DependencyObject : winrt::impl::abi_t<winrt::Microsoft::UI::Xaml::IDependencyObject>
		{
			virtual HRESULT STDMETHODCALLTYPE GetValue(
				const CDependencyProperty* pDependecyProperty,
				IInspectable** ppValue
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE PrepareState() = 0;
			virtual bool STDMETHODCALLTYPE IsExternalObjectReference() = 0;
			virtual HRESULT STDMETHODCALLTYPE OnParentUpdated(
				CDependencyObject* pOldParentCore,
				CDependencyObject* pNewParentCore,
				bool isNewParentAlive
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnTreeParentUpdated(CDependencyObject* pNewParent, bool isParentAlive) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnCollectionChanged(unsigned int nCollectionChangeType, unsigned int nIndex) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnInheritanceContextChanged() = 0;
			virtual HRESULT STDMETHODCALLTYPE DisconnectFrameworkPeerCore() = 0;
			virtual HRESULT STDMETHODCALLTYPE GetDefaultValue2(const CDependencyProperty* pDependencyObject, CValue* pValue) = 0;
			virtual HRESULT STDMETHODCALLTYPE EnterImpl(
				BOOL bLive,
				BOOL bSkipNameRegistration,
				BOOL bCoercedIsEnabled
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE LeaveImpl(
				BOOL bLive,
				BOOL bSkipNameRegistration,
				BOOL bCoercedIsEnabled
			) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnPropertyChanged2(PropertyChangedParams& args) = 0;
			virtual HRESULT STDMETHODCALLTYPE OnChildUpdated(DirectUI_DependencyObject* pChild) = 0;
			virtual HRESULT STDMETHODCALLTYPE NotifyDeferredElementStateChanged(
				KnownPropertyIndex propertyIndex,
				DeferredElementStateChange state,
				unsigned int collectionIndex
			) = 0;
		};


		namespace MyDirectUI_DependencyObject
		{
			HRESULT STDMETHODCALLTYPE EnterImpl(
				DirectUI_DependencyObject* This,
				BOOL bLive,
				BOOL bSkipNameRegistration,
				BOOL bCoercedIsEnabled
			);
			HRESULT STDMETHODCALLTYPE LeaveImpl(
				DirectUI_DependencyObject* This,
				BOOL bLive,
				BOOL bSkipNameRegistration,
				BOOL bCoercedIsEnabled
			);

			void HookAttach(DirectUI_DependencyObject* This);
			void HookDetach(DirectUI_DependencyObject* This);
			void DetachAllHooks();

			using DirectUI_DependencyObjectOrgs = std::tuple<
				size_t,
				decltype(&EnterImpl),
				decltype(&LeaveImpl)
			>;
			std::unordered_map<
				void**,
				DirectUI_DependencyObjectOrgs
			> g_hookedObject{};
		}

		HRESULT WINAPI MyDirectUI_DXamlCore_GetPeerPrivate(
			DirectUI_DXamlCore* This,
			IInspectable* pOuter,
			CDependencyObject* pDependencyObject,
			DirectUI_DXamlCore_GetPeerPrivateCreateMode createMode,
			KnownTypeIndex hClass,
			BOOL bInternalRef,
			BOOL fCreatePegged,
			BOOL* pIsPendingDelete,
			DirectUI_DependencyObject** ppObject
		);

		UCHAR g_DirectUI_DXamlCore_TryGetPeer_AsmCode[]
		{
			0x48, 0x89, 0x5C, 0x24, 0x20,					// mov     [rsp+arg_18], rbx
			0x55,											// push    rbp
			0x56,											// push    rsi
			0x57,											// push    rdi
			0x41, 0x54,										// push    r12
			0x41, 0x55,										// push    r13
			0x41, 0x56,										// push    r14
			0x41, 0x57,										// push    r15
			0x48, 0x83, 0xEC, 0x60,							// sub     rsp, 60h
			0x48, 0x8B, 0x05, 0XFF, 0XFF, 0XFF, 0XFF,		// mov     rax, cs:__security_cookie
			0x48, 0x33, 0xC4,								// xor	   rax, rsp
			0x48, 0x89, 0x44, 0x24, 0x50,					// mov     [rsp+98h+var_48], rax
			0x4C, 0x8B, 0xAC, 0x24, 0xD8, 0x00, 0x00, 0x00, // mov     r13, [rsp+98h+arg_38]
			0x45, 0x33, 0xE4,								// xor	   r12d, r12d
			0x4C, 0x8B, 0xB4, 0x24, 0xE0, 0x00, 0x00, 0x00, // mov     r14,[rsp + 98h + arg_40]
			0x41, 0x8B, 0xC1,								// mov     eax, r9d
			0x89, 0x44, 0x24, 0x38,							// mov     dword ptr[rsp + 98h + var_60], eax
			0x4D, 0x8B, 0xF8,								// mov     r15, r8
			0x48, 0x89, 0x54, 0x24, 0x40,					// mov     [rsp + 98h + var_58], rdx
			0x48, 0x8B, 0xF9,								// mov     rdi, rcx
			0x41, 0x8B, 0xEC,								// mov     ebp, r12d
			0x41, 0x8B, 0xDC,								// mov     ebx, r12d
			0x4D, 0x85, 0xC0,								// test    r8, r8
			0x0F, 0x84, 0XFF, 0XFF, 0XFF, 0XFF				// jz      xxxx
		};
		UCHAR g_DirectUI_DXamlCore_TryGetPeer_AsmCode_WAR1P1[]
		{
			0x48, 0x89, 0x5C, 0x24, 0x08,					// mov     [rsp+arg_0], rbx
			0x48, 0x89, 0x5C, 0x24, 0x20,					// mov     [rsp+arg_18], rbx
			0x48, 0x89, 0x54, 0x24, 0x10,					// mov     [rsp+arg_8], rdx
			0x55,											// push    rbp
			0x56,											// push    rsi
			0x57,											// push    rdi
			0x41, 0x54,										// push    r12
			0x41, 0x55,										// push    r13
			0x41, 0x56,										// push    r14
			0x41, 0x57,										// push    r15
			0x48, 0x83, 0xEC, 0x50,							// sub     rsp, 50h
			0x45, 0x33, 0xFF,								// xor     r15d, r15d
			0x41, 0x8B, 0xC1,								// mov     eax, r9d
			0x4D, 0x8B, 0xE0,								// mov     r12, r8
			0x48, 0x8B, 0xF9,								// mov     rdi, rcx
			0x41, 0x8B, 0xEF,								// mov     ebp, r15d
			0x41, 0x8B, 0xDF,								// mov     ebx, r15d
			0x4D, 0x85, 0xC0,								// test    r8, r8
			0x0F, 0x84, 0XFF, 0XFF, 0XFF, 0XFF,				// jz      xxxx
			0x4C, 0x8B, 0xB4, 0x24, 0xD0, 0x00, 0x00, 0x00, // mov     r14,[rsp + 88h + arg_40]
			0x4D, 0x85, 0xF6,								// test    r14, r14
			0x0F, 0x84, 0XFF, 0XFF, 0XFF, 0XFF,				// jz      xxxx
			0x4C, 0x8B, 0xAC, 0x24, 0xC8, 0x00, 0x00, 0x00, // mov     r13, [rsp+88h+arg_38]
			0x4D, 0x89, 0x3E,								// mov	   [r14], r15
			0x4D, 0x85, 0xED,								// test    r13, r13
			0x0F, 0x85, 0XFF, 0XFF, 0XFF, 0XFF				// jnz     xxxx
		};


		UCHAR g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode[]
		{
			0x48, 0x89, 0x5C, 0x24, 0x08,						// mov     [rsp+arg_0], rbx
			0x57,												// push    rdi
			0x48, 0x83, 0xEC, 0x50,								// sub     rsp, 50h
			0x48, 0x8B, 0x05, 0XFF, 0XFF, 0XFF, 0XFF,			// mov     rax, cs:__security_cookie
			0x48, 0x33, 0xC4,									// xor     rax, rsp
			0x48, 0x89, 0x44, 0x24, 0x48,						// mov     [rsp+58h+var_10], rax
			0x41, 0x8B, 0xF8,									// mov     edi, threadId
			0x48, 0x8B, 0xDA,									// mov     rbx, rdx
			0x48, 0x89, 0x54, 0x24, 0x30,						// mov     [rsp+58h+var_28], rdx
			0x83, 0x64, 0x24, 0x20, 0x00,						// and     [rsp+58h+var_38], 0
			0x33, 0xC0,											// xor     eax, eax
			0x48, 0x89, 0x02,									// mov     [rdx], rax
			0x48, 0x21, 0x02,									// and     [rdx], rax
			0xC7, 0x44, 0x24, 0x20, 0x01, 0x00, 0x00, 0x00,		// mov     [rsp+58h+var_38], 1
			0xE8, 0XFF, 0XFF, 0XFF, 0XFF,						// call    ?GetCurrentNoRef@FrameworkApplication@DirectUI@@SAPEAV12@XZ ; DirectUI::FrameworkApplication::GetCurrentNoRef(void)
			0x48, 0x85, 0xC0,									// test    rax, rax
			0x0F, 0x84, 0x95, 0x00, 0x00, 0x00,					// jz      xxxx
			0x48, 0x83, 0x64, 0x24, 0x40, 0x00,					// and     [rsp+58h+debugSettings.ptr_], 0
			0x48, 0x8D, 0x48, 0x58,								// lea     rcx, [rax+58h]
			0x48, 0x8B, 0x01,									// mov     rax, [rcx]
			0x48, 0x8D, 0x54, 0x24, 0x40,						// lea     rdx, [rsp+58h+debugSettings]
			0x48, 0x8B, 0x40, 0x40								// mov     rax, [rax+40h]
		};
		UCHAR g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode_1P1[]
		{
			0x48, 0x8B, 0xC4,							// mov     rax, rsp
			0x48, 0x89, 0x58, 0x18,						// mov     [rax+18h], rbx
			0x48, 0x89, 0x50, 0x10,						// mov     [rax+10h], rdx
			0x48, 0x89, 0x48, 0x08,						// mov     [rax+8], rcx
			0x57,										// push    rdi
			0x48, 0x83, 0xEC, 0x30,						// sub     rsp, 30h
			0x41, 0x8B, 0xF8,							// mov     edi, r8d
			0x48, 0x8B, 0xDA,							// mov     rbx, rdx
			0x83, 0x60, 0xE8, 0x00,						// and     dword ptr [rax-18h], 0
			0x48, 0x83, 0x22, 0x00,						// and     qword ptr [rdx], 0
			0xC7, 0x40, 0xE8, 0x01, 0x00, 0x00, 0x00,	// mov     dword ptr [rax-18h], 1
			0xE8, 0x9E, 0x03, 0xA6, 0xFF,				// call    ?GetCurrentNoRef@FrameworkApplication@DirectUI@@SAPEAV12@XZ ; DirectUI::FrameworkApplication::GetCurrentNoRef(void)
			0x48, 0x85, 0xC0,							// test    rax, rax
			0x0F, 0x84, 0x95, 0x00, 0x00, 0x00,			// jz      xxxx
			0x48, 0x83, 0x64, 0x24, 0x40, 0x00,			// and     [rsp+38h+arg_0], 0
			0x48, 0x8D, 0x48, 0x58,						// lea     rcx, [rax+58h]
			0x48, 0x8B, 0x01,							// mov     rax, [rcx]
			0x48, 0x8D, 0x54, 0x24, 0x40,				// lea     rdx, [rsp+38h+arg_0]
			0x48, 0x8B, 0x40, 0x40						// mov     rax, [rax+40h]
		};

		constexpr size_t DirectUI_DependencyObject_EnterImpl_VTableIndex{ 24 };
		HookHelper::OffsetStorage g_DirectUI_DXamlCore_GetPeerPrivate_Offset{ 0 };
		HookHelper::OffsetStorage g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset{ 0 };
		decltype(&MyDirectUI_DXamlCore_GetPeerPrivate) g_actualDirectUI_DXamlCore_GetPeerPrivate{ nullptr };
		bool g_hookEnabled{ false };

		void EnableGetPeerPrivateHook(bool enable)
		{
			if (g_actualDirectUI_DXamlCore_GetPeerPrivate)
			{
				HookHelper::Detours::Write([&]()
				{
					if (enable)
					{
						HookHelper::Detours::Attach(reinterpret_cast<PVOID*>(&g_actualDirectUI_DXamlCore_GetPeerPrivate), MyDirectUI_DXamlCore_GetPeerPrivate);
					}
					else
					{
						HookHelper::Detours::Detach(reinterpret_cast<PVOID*>(&g_actualDirectUI_DXamlCore_GetPeerPrivate), MyDirectUI_DXamlCore_GetPeerPrivate);
					}
				});

				g_hookEnabled = enable;
			}
		}
		void TryEnableGetPeerPrivateHook(PVOID baseAddress, bool enable)
		{
			auto [startAddress, maxSearchBytes] {HookHelper::GetTextSectionInfo(baseAddress)};
			if (!g_DirectUI_DXamlCore_GetPeerPrivate_Offset.IsValid())
			{
				g_DirectUI_DXamlCore_GetPeerPrivate_Offset = HookHelper::OffsetStorage::From(
					baseAddress,
					HookHelper::MatchAsmCode(
						reinterpret_cast<PUCHAR>(baseAddress),
						{
							std::make_pair(
								g_DirectUI_DXamlCore_TryGetPeer_AsmCode,
								sizeof(g_DirectUI_DXamlCore_TryGetPeer_AsmCode)
							)
						},
						std::vector
						{
							std::make_pair(
								23ull,
								4ull
							),
							std::make_pair(
								56ull,
								4ull
							)
						},
						maxSearchBytes
					)
				);
				if (!g_DirectUI_DXamlCore_GetPeerPrivate_Offset.IsValid())
				{
					g_DirectUI_DXamlCore_GetPeerPrivate_Offset = HookHelper::OffsetStorage::From(
						baseAddress,
						HookHelper::MatchAsmCode(
							reinterpret_cast<PUCHAR>(baseAddress),
							{
								std::make_pair(
									g_DirectUI_DXamlCore_TryGetPeer_AsmCode_WAR1P1,
									sizeof(g_DirectUI_DXamlCore_TryGetPeer_AsmCode_WAR1P1)
								)
							},
							std::vector
							{
								std::make_pair(
									53ull,
									4ull
								),
								std::make_pair(
									13ull,
									4ull
								),
								std::make_pair(
									16ull,
									4ull
								)
							},
							maxSearchBytes
						)
					);
				}
			}


			if (!g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset.IsValid())
			{
				g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset = HookHelper::OffsetStorage::From(
					baseAddress,
					HookHelper::MatchAsmCode(
						reinterpret_cast<PUCHAR>(baseAddress),
						{
							std::make_pair(
								g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode,
								sizeof(g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode)
							)
						},
						std::vector
						{ 
							std::make_pair(
								13ull,
								4ull
							),
							std::make_pair(
								41ull,
								4ull
							),
							std::make_pair(
								31ull,
								0ull
							)
						},
						maxSearchBytes
					)
				);
				if (!g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset.IsValid())
				{
					g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset = HookHelper::OffsetStorage::From(
						baseAddress,
						HookHelper::MatchAsmCode(
							reinterpret_cast<PUCHAR>(baseAddress),
							{
								std::make_pair(
									g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode_1P1,
									sizeof(g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode_1P1)
								)
							},
							std::vector
							{
								std::make_pair(
									sizeof(g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode_1P1),
									0ull
								)
							},
							maxSearchBytes
							)
					);
				}
			}
			if (!MUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset.IsValid())
			{
				MessageBoxW(0, L"MUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset", 0, 0);
			}

			MUX::g_actualDirectUI_DXamlCore_GetPeerPrivate = MUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset.To<decltype(MUX::g_actualDirectUI_DXamlCore_GetPeerPrivate)>(baseAddress);
			MUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId = MUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset.To<decltype(MUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId)>(baseAddress);
			EnableGetPeerPrivateHook(enable);
		}
	}

	bool g_startup{ false };
	PVOID g_cookie{ nullptr };
	VOID CALLBACK LdrDllNotification(
		ULONG notificationReason,
		HookHelper::PCLDR_DLL_NOTIFICATION_DATA notificationData,
		PVOID context
	);
}

void DiagnosticsHooks::WUX::MyDirectUI_DependencyObject::HookDetach(WUX::DirectUI_DependencyObject* This)
{
	auto vtable{ HookHelper::GetObjectVTable(This) };
	if (!vtable)
	{
		return;
	}
	if (g_hookedObject.find(vtable) == g_hookedObject.end())
	{
		return;
	}

	auto [refCount, enterImpl, leaveImpl] { g_hookedObject.at(vtable) };

	if (!(--refCount))
	{
		HookHelper::WriteMemory(&vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex], [&]
		{
			vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex] = Utils::union_cast<void*>(enterImpl);
		});
		HookHelper::WriteMemory(&vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1], [&]
		{
			vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1] = Utils::union_cast<void*>(leaveImpl);
		});

		g_hookedObject.erase(vtable);
	}
}
void DiagnosticsHooks::WUX::MyDirectUI_DependencyObject::HookAttach(WUX::DirectUI_DependencyObject* This)
{
	auto vtable{ HookHelper::GetObjectVTable(This) };
	if (!vtable)
	{
		return;
	}
	if (g_hookedObject.find(vtable) != g_hookedObject.end())
	{
		std::get<0>(g_hookedObject.at(vtable)) += 1;
		return;
	}

	DirectUI_DependencyObjectOrgs hookInfo{};
	auto& [refCount, enterImpl, leaveImpl] { hookInfo };

	refCount = 1;
	HookHelper::WriteMemory(&vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex], [&]
	{
		enterImpl = Utils::union_cast<decltype(&WUX::MyDirectUI_DependencyObject::EnterImpl)>(vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex]);
		vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex] = Utils::union_cast<void*>(&WUX::MyDirectUI_DependencyObject::EnterImpl);
	});
	HookHelper::WriteMemory(&vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1], [&]
	{
		leaveImpl = Utils::union_cast<decltype(&WUX::MyDirectUI_DependencyObject::LeaveImpl)>(vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1]);
		vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1] = Utils::union_cast<void*>(&WUX::MyDirectUI_DependencyObject::LeaveImpl);
	});

	g_hookedObject.insert_or_assign(vtable, hookInfo);
}
void DiagnosticsHooks::WUX::MyDirectUI_DependencyObject::DetachAllHooks()
{
	for (auto& [vtable, originalFunctions] : g_hookedObject)
	{
		auto [refCount, enterImpl, leaveImpl] {originalFunctions};

		HookHelper::WriteMemory(&vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex], [&]
		{
			vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex] = Utils::union_cast<void*>(enterImpl);
		});
		HookHelper::WriteMemory(&vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1], [&]
		{
			vtable[WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1] = Utils::union_cast<void*>(leaveImpl);
		});
	}
	g_hookedObject.clear();
}
void DiagnosticsHooks::MUX::MyDirectUI_DependencyObject::HookDetach(MUX::DirectUI_DependencyObject* This)
{
	auto vtable{ HookHelper::GetObjectVTable(This) };
	if (!vtable)
	{
		return;
	}
	if (g_hookedObject.find(vtable) == g_hookedObject.end())
	{
		return;
	}

	auto [refCount, enterImpl, leaveImpl] { g_hookedObject.at(vtable) };

	if (!(--refCount))
	{
		HookHelper::WriteMemory(&vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex], [&]
			{
				vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex] = Utils::union_cast<void*>(enterImpl);
			});
		HookHelper::WriteMemory(&vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1], [&]
			{
				vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1] = Utils::union_cast<void*>(leaveImpl);
			});

		g_hookedObject.erase(vtable);
	}
}
void DiagnosticsHooks::MUX::MyDirectUI_DependencyObject::HookAttach(MUX::DirectUI_DependencyObject* This)
{
	auto vtable{ HookHelper::GetObjectVTable(This) };
	if (!vtable)
	{
		return;
	}
	if (g_hookedObject.find(vtable) != g_hookedObject.end())
	{
		std::get<0>(g_hookedObject.at(vtable)) += 1;
		return;
	}

	DirectUI_DependencyObjectOrgs hookInfo{};
	auto& [refCount, enterImpl, leaveImpl] { hookInfo };

	refCount = 1;
	HookHelper::WriteMemory(&vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex], [&]
	{
		enterImpl = Utils::union_cast<decltype(&MUX::MyDirectUI_DependencyObject::EnterImpl)>(vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex]);
		vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex] = Utils::union_cast<void*>(&MUX::MyDirectUI_DependencyObject::EnterImpl);
	});
	HookHelper::WriteMemory(&vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1], [&]
	{
		leaveImpl = Utils::union_cast<decltype(&MUX::MyDirectUI_DependencyObject::LeaveImpl)>(vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1]);
		vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1] = Utils::union_cast<void*>(&MUX::MyDirectUI_DependencyObject::LeaveImpl);
	});

	g_hookedObject.insert_or_assign(vtable, hookInfo);
}
void DiagnosticsHooks::MUX::MyDirectUI_DependencyObject::DetachAllHooks()
{
	for (auto& [vtable, originalFunctions] : g_hookedObject)
	{
		auto [refCount, enterImpl, leaveImpl] {originalFunctions};

		HookHelper::WriteMemory(&vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex], [&]
		{
			vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex] = Utils::union_cast<void*>(enterImpl);
		});
		HookHelper::WriteMemory(&vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1], [&]
		{
			vtable[MUX::DirectUI_DependencyObject_EnterImpl_VTableIndex + 1] = Utils::union_cast<void*>(leaveImpl);
		});
	}
	g_hookedObject.clear();
}

HRESULT STDMETHODCALLTYPE DiagnosticsHooks::WUX::MyDirectUI_DependencyObject::EnterImpl(
	DirectUI_DependencyObject* This,
	BOOL bLive,
	BOOL bSkipNameRegistration,
	BOOL bCoercedIsEnabled
)
{
	HRESULT hr{ (std::get<1>(g_hookedObject.at(HookHelper::GetObjectVTable(This))))(This, bLive, bSkipNameRegistration, bCoercedIsEnabled) };

	if (SUCCEEDED(hr) && bLive)
	{
		DiagnosticsHandler::OnVisualTreeChanged(
			reinterpret_cast<IInspectable*>(This),
			DiagnosticsHandler::FrameworkType::UWP,
			DiagnosticsHandler::MutationType::Add
		);
	}
	HookDetach(This);

	return hr;
}
HRESULT STDMETHODCALLTYPE DiagnosticsHooks::WUX::MyDirectUI_DependencyObject::LeaveImpl(
	DirectUI_DependencyObject* This,
	BOOL bLive,
	BOOL bSkipNameRegistration,
	BOOL bCoercedIsEnabled
)
{
	auto ptr{ std::get<2>(g_hookedObject.at(HookHelper::GetObjectVTable(This))) };
	DiagnosticsHandler::OnVisualTreeChanged(
		reinterpret_cast<IInspectable*>(This),
		DiagnosticsHandler::FrameworkType::UWP,
		DiagnosticsHandler::MutationType::Remove
	);
	HookDetach(This);

	return ptr(This, bLive, bSkipNameRegistration, bCoercedIsEnabled);
}

HRESULT STDMETHODCALLTYPE DiagnosticsHooks::MUX::MyDirectUI_DependencyObject::EnterImpl(
	DirectUI_DependencyObject* This,
	BOOL bLive,
	BOOL bSkipNameRegistration,
	BOOL bCoercedIsEnabled
)
{
	HRESULT hr{ (std::get<1>(g_hookedObject.at(HookHelper::GetObjectVTable(This))))(This, bLive, bSkipNameRegistration, bCoercedIsEnabled) };

	if (SUCCEEDED(hr) && bLive)
	{
		DiagnosticsHandler::OnVisualTreeChanged(
			reinterpret_cast<IInspectable*>(This),
			DiagnosticsHandler::FrameworkType::WinUI,
			DiagnosticsHandler::MutationType::Add
		);
	}
	HookDetach(This);

	return hr;
}
HRESULT STDMETHODCALLTYPE DiagnosticsHooks::MUX::MyDirectUI_DependencyObject::LeaveImpl(
	DirectUI_DependencyObject* This,
	BOOL bLive,
	BOOL bSkipNameRegistration,
	BOOL bCoercedIsEnabled
)
{
	auto ptr{ (std::get<2>(g_hookedObject.at(HookHelper::GetObjectVTable(This)))) };
	DiagnosticsHandler::OnVisualTreeChanged(
		reinterpret_cast<IInspectable*>(This),
		DiagnosticsHandler::FrameworkType::WinUI,
		DiagnosticsHandler::MutationType::Remove
	);
	HookDetach(This);

	return ptr(This, bLive, bSkipNameRegistration, bCoercedIsEnabled);
}

HRESULT WINAPI DiagnosticsHooks::WUX::MyDirectUI_DXamlCore_GetPeerPrivate(
	DirectUI_DXamlCore* This,
	IInspectable* pOuter,
	CDependencyObject* pDependencyObject,
	DirectUI_DXamlCore_GetPeerPrivateCreateMode createMode,
	KnownTypeIndex hClass,
	BOOL bInternalRef,
	BOOL fCreatePegged,
	BOOL* pIsPendingDelete,
	DirectUI_DependencyObject** ppObject
)
{
	HRESULT hr
	{
		g_actualDirectUI_DXamlCore_GetPeerPrivate(
			This,
			pOuter,
			pDependencyObject,
			createMode,
			hClass,
			bInternalRef,
			fCreatePegged,
			pIsPendingDelete,
			ppObject
		)
	};

	if (
		FAILED(hr) ||
		!pDependencyObject ||
		!ppObject ||
		!(*ppObject) ||
		pOuter ||
		pIsPendingDelete ||
		bInternalRef ||
		fCreatePegged ||
		createMode != DirectUI_DXamlCore_GetPeerPrivateCreateMode::GetOnly
		)
	{
		return hr;
	}
	MyDirectUI_DependencyObject::HookAttach(*ppObject);

	return hr;
}

HRESULT WINAPI DiagnosticsHooks::MUX::MyDirectUI_DXamlCore_GetPeerPrivate(
	DirectUI_DXamlCore* This,
	IInspectable* pOuter,
	CDependencyObject* pDependencyObject,
	DirectUI_DXamlCore_GetPeerPrivateCreateMode createMode,
	KnownTypeIndex hClass,
	BOOL bInternalRef,
	BOOL fCreatePegged,
	BOOL* pIsPendingDelete,
	DirectUI_DependencyObject** ppObject
)
{
	HRESULT hr
	{
		g_actualDirectUI_DXamlCore_GetPeerPrivate(
			This,
			pOuter,
			pDependencyObject,
			createMode,
			hClass,
			bInternalRef,
			fCreatePegged,
			pIsPendingDelete,
			ppObject
		)
	};

	if (
		FAILED(hr) ||
		!pDependencyObject ||
		!ppObject ||
		!(*ppObject) ||
		pOuter ||
		pIsPendingDelete ||
		bInternalRef ||
		fCreatePegged ||
		createMode != DirectUI_DXamlCore_GetPeerPrivateCreateMode::GetOnly
	)
	{
		return hr;
	}
	MyDirectUI_DependencyObject::HookAttach(*ppObject);

	return hr;
}

void DiagnosticsHooks::Prepare()
{
	if (
		WUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset.IsValid() &&
		WUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset.IsValid() &&
		WUX::DirectUI_DependencyObject_EnterImpl_VTableIndex
	)
	{
		return;
	}

	{
		auto wuxModule{ HookHelper::ImageMapper(LR"(C:\Windows\System32\Windows.UI.Xaml.dll)") };
		auto [startAddress, maxSearchBytes]{HookHelper::GetTextSectionInfo(wuxModule.GetBaseAddress())};
		
		WUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset = HookHelper::OffsetStorage::From(
			wuxModule.GetBaseAddress(),
			HookHelper::MatchAsmCode(
				wuxModule.GetBaseAddress<PUCHAR>(),
				{
					std::make_pair(
						WUX::g_DirectUI_DXamlCore_TryGetPeer_AsmCode,
						sizeof(WUX::g_DirectUI_DXamlCore_TryGetPeer_AsmCode)
					)
				},
				std::vector
				{
					std::make_pair(
						29ull,
						1ull
					),
					std::make_pair(
						5ull,
						1ull
					),
					std::make_pair(
						1ull,
						2ull
					),
					std::make_pair(
						1ull,
						1ull
					)
				},
				maxSearchBytes
			)
		);
		if (!WUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset.IsValid())
		{
			WUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset = HookHelper::OffsetStorage::From(
				wuxModule.GetBaseAddress(),
				HookHelper::MatchAsmCode(
					wuxModule.GetBaseAddress<PUCHAR>(),
					{
						std::make_pair(
							WUX::g_DirectUI_DXamlCore_TryGetPeer_AsmCode_Win10,
							sizeof(WUX::g_DirectUI_DXamlCore_TryGetPeer_AsmCode_Win10)
						)
					},
					std::vector
					{
						std::make_pair(
							sizeof(WUX::g_DirectUI_DXamlCore_TryGetPeer_AsmCode_Win10) - 4ull,
							4ull
						)
					},
					maxSearchBytes
				)
			);
		}

		if (!WUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset.IsValid())
		{
			MessageBoxW(0, L"WUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset", 0, 0);
		}



		WUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset = HookHelper::OffsetStorage::From(
			wuxModule.GetBaseAddress(),
			HookHelper::MatchAsmCode(
				wuxModule.GetBaseAddress<PUCHAR>(),
				{
					std::make_pair(
						WUX::g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode,
						sizeof(WUX::g_Diagnostics_DiagnosticsInterop_GetDispatcherQueueForThreadId_AsmCode)
					)
				},
				std::vector
				{
					std::make_pair(
						51ull,
						4ull
					),
					std::make_pair(
						3ull,
						0ull
					),
				},
				maxSearchBytes
			)
		);

		if (!WUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset.IsValid())
		{
			MessageBoxW(0, L"WUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset", 0, 0);
		}
	}
}
void DiagnosticsHooks::Startup()
{
	if (g_startup)
	{
		return;
	}

	auto wuxModule{ GetModuleHandleW(L"Windows.UI.Xaml.dll") };
	if (wuxModule)
	{
		WUX::g_actualDirectUI_DXamlCore_GetPeerPrivate = WUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset.To<decltype(WUX::g_actualDirectUI_DXamlCore_GetPeerPrivate)>(wuxModule);
		WUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId = WUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset.To<decltype(WUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId)>(wuxModule);
		WUX::EnableGetPeerPrivateHook(true);
	}

	auto muxModule{ DiagnosticsHandler::GetMUXHandle() };
	if (muxModule)
	{
		MUX::TryEnableGetPeerPrivateHook(muxModule, true);
	}

	g_startup = true;
}
void DiagnosticsHooks::Shutdown()
{
	if (!g_startup)
	{
		return;
	}

	auto muxModule{DiagnosticsHandler::GetMUXHandle()};
	if (muxModule)
	{
		MUX::EnableGetPeerPrivateHook(false);
		MUX::MyDirectUI_DependencyObject::DetachAllHooks();
	}

	auto wuxModule{ GetModuleHandleW(L"Windows.UI.Xaml.dll") };
	if (wuxModule)
	{
		WUX::EnableGetPeerPrivateHook(false);
		WUX::MyDirectUI_DependencyObject::DetachAllHooks();
	}

	g_startup = false;
}

VOID CALLBACK DiagnosticsHooks::LdrDllNotification(
	ULONG notificationReason,
	HookHelper::PCLDR_DLL_NOTIFICATION_DATA notificationData,
	PVOID context
)
{
	if (notificationReason == HookHelper::LDR_DLL_NOTIFICATION_REASON_LOADED)
	{
		if (!_wcsicmp(L"Windows.UI.Xaml.dll", notificationData->Loaded.BaseDllName->Buffer))
		{
			WUX::g_actualDirectUI_DXamlCore_GetPeerPrivate = WUX::g_DirectUI_DXamlCore_GetPeerPrivate_Offset.To<decltype(WUX::g_actualDirectUI_DXamlCore_GetPeerPrivate)>(notificationData->Loaded.DllBase);
			WUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId = WUX::g_DiagnosticsInterop_GetDispatcherQueueForThreadId_Offset.To<decltype(WUX::g_actualDiagnosticsInterop_GetDispatcherQueueForThreadId)>(notificationData->Loaded.DllBase);
			WUX::EnableGetPeerPrivateHook(true);
		}
		if (DiagnosticsHandler::IsMUXModule(reinterpret_cast<HMODULE>(notificationData->Loaded.DllBase)))
		{
			MUX::TryEnableGetPeerPrivateHook(notificationData->Loaded.DllBase, true);
		}
	}
}