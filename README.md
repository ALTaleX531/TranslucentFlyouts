> [!NOTE] 
> The project is no longer active! 
>   
> Dear Users,
> 
> It is with a heavy heart that I announce that the TranslucentFlyouts project is no longer actively developed and receive updates.   
> 
> Why?   
> 
> 1. TranslucentFlyouts has so many features that it is very expensive to maintain. However, there are still more requests for more features.
> 2. Since users will mostly use TranslucentFlyouts in combination with other appearance tweaks, this means that I can't use an implementation as invasive as StartAllBack.
> 3. Microsoft is further trampling on msstyle, which makes full coverage of vanilla rendering a necessity, yet libraries that do high-quality 2D rendering almost always introduce multithreading issues.
> 4. The current implementation of injecting into global applications is seriously buggy, it causes .NET applications to crash however the memory dump file generated is useless and I can't even figure out why it crashes. In addition to this, it crashes some games that have strict anti-cheat features. Unfortunately, I haven't found a stable and reliable way to inject globally.  
> 
> There are actually much more issues than you think, but I don't have a nice way to present them all. But I'd say the most important reason is that I don't have that much time to develop it anymore.   
>   
> Currently TranslucentFlyouts still works without problems on most systems, only a few settings need to be [tweaked](https://github.com/ALTaleX531/TranslucentFlyouts/files/14159185/disable_internet_connection.zip), such as preventing TranslucentFlyouts from downloading symbols that may cause failures, these symbol files are mostly referenced by features that you are unlikely to use.   
> 
> I may create a lighter and more stable project to take over TranslucentFlyouts in the future, but who knows when...?
>  
> I want to express my deepest gratitude for your support and for choosing TranslucentFlyouts. Your enthusiasm and feedback have been invaluable throughout this journey. It has been an immense honor for me to contribute to a project that aimed to enhance your user experience. If TranslucentFlyouts has caused any inconvenience or disruption to your Windows setup, I sincerely apologize.
# TranslucentFlyouts
An application that makes most of the win32 popup menus translucent/transparent on Windows 10/11, providing more options to tweak it to meet your need.

Compared to V1, TranslucentFlyouts V2 has better compatibility and the ability to customize.   
TranslucentFlyouts V1 has been moved to [TranslucentFlyoutsV1](https://github.com/ALTaleX531/TranslucentFlyoutsV1).   
**TranslucentFlyouts uses [LGNU V3 license](./COPYING.LESSER) started from V2.** 

**I' m developing this program entirely in my spare time without any profit, so it may not get timely updates due to my busy academic schedule. I hope you can understand.**

[![license](https://img.shields.io/github/license/ALTaleX531/TranslucentFlyouts.svg)](https://www.gnu.org/licenses/lgpl-3.0.en.html)
[![Github All Releases](https://img.shields.io/github/downloads/ALTaleX531/TranslucentFlyouts/total.svg)](https://github.com/ALTaleX531/TranslucentFlyouts/releases)
[![GitHub release](https://img.shields.io/github/release/ALTaleX531/TranslucentFlyouts.svg)](https://github.com/ALTaleX531/TranslucentFlyouts/releases/latest)
<img src="https://img.shields.io/badge/language-c++-F34B7D.svg"/>
<img src="https://img.shields.io/github/last-commit/ALTaleX531/TranslucentFlyouts.svg"/>  

###  Other Languages
[简体中文](./ReadMe/zh-cn.md)  
## Catalog
- [Gallery](#gallery)
- [How to use](#how-to-use)
- [How to configure](#how-to-configure)
- [Limitations & Compatibility](#limitations-and-compatibility)
- [Dependencies & References](#dependencies-and-references)
## Gallery

<details><summary><b>Acrylic</b></summary>

Windows 10   
![Windows10 Light Mode](./Images/Acrylic/LightMode_Windows10.png)
![Windows10 Dark Mode](./Images/Acrylic/DarkMode_Windows10.png)

Windows 11  
![Windows11 Light Mode](./Images/Acrylic/LightMode_Windows11.png)
![Windows11 Dark Mode](./Images/Acrylic/DarkMode_Windows11.png)
</details>

<details><summary><b>Mica/MicaAlt (Windows 11 Only)</b></summary>

> ![MicaAlt](./Images/Mica/DarkMode_Windows11(MicaAlt).png)
</details>

<details><summary><b>Custom Rendering Sample</b></summary>

![Sample 1](./Images/CustomRendering/LightMode_Sample1.png)
![Sample 2](./Images/CustomRendering/LightMode_Sample2.png)
</details>

<details><summary><b>Fluent Animations</b></summary>

![Sample 1](./Images/FluentAnimations/Sample1.gif)   

![Sample 2](./Images/FluentAnimations/Sample2.gif)   

</details>

## How to use

### Install
1. Download the compiled program archive from the [Release](https://github.com/ALTaleX531/TranslucentFlyouts/releases/latest) page.
2. Unzip it to a location such as "`C:\Program Files`".
3. Run "`install.cmd`" as administrator.

> [Note!]:   
> **Downloading symbol files from Microsoft server is required at the first time or after a windows update, otherwise some functionalities will be unavailable!**  

### Uninstall
1. Run "`uninstall.cmd`" as administrator.
2. Delete the remaining files. (It is recommended to logoff before doing it)

## How to configure
Thanks to **[@Satanarious](https://github.com/Satanarious/)**'s hard work, a perfect and easy-to-use GUI can be found [here](https://github.com/Satanarious/TransparentFlyoutsConfig)!  
[Take me to the download page!](https://github.com/Satanarious/TransparentFlyoutsConfig/releases/latest)   

Of course, you can also modify related registry settings on your own according to the [configuration docs](./Config/en-us/CONFIG.md) .

## Limitations and Compatibility
### Here are some situations that TranslucentFlyouts will always be automatically disabled.
#### 1. Windows 2000 Style popup menu  
![Windows2000](./Images/Unsupported/Windows2000.png)

Outdated.   
Some third-party applications like `HoneyView` may cause this issue.
#### 2. Ownerdrawn popup menu
![Ownerdrawn](./Images/Unsupported/Ownerdrawn.png)

As you can see, it is a QT popup menu.  
It really looks like the default menu, isn't it?  
But it's rendering procedure is completely different from the defualt one, making TranslucentFlyout hard to modify its visual content.  
#### **3. StartAllBack**
`StartAllBack` has built-in support for translucent popup menu, and its rendering procedure priority is higher than TranslucentFlyouts.  
> [Note!]:  
> For this reason, `v2.0.0-alpha.4` and higher versions of TranslucentFlyouts will automatically disable itself when `StartAllBack` was detected, unless there is a way to fully disable `StartAllBack`' s handling procedure.  
> However, TranslucentFlyouts is still available for other applications except `Explorer`.

**It may possibly also cause this issue when StartAllBack is installed**  
![StartAllBack_MenuItemWithFlaws](./Images/StartAllBack/MenuItemWithFlaws.png)
![StartAllBack_MenuItemColoredWithFlaws](./Images/StartAllBack/MenuItemColoredWithFlaws.png)   
As you can see there is a white border around the menu item, it will always exist until you disable or uninstall StartAllBack...  
For now, there are two ways to solve this.   
1. Set the dword value `EnableCustomRendering` to non-zero for TranslucentFlyouts.   
Registry path: HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\EnableCustomRendering
2. Install a custom theme.

## Dependencies and References
### [Microsoft Research Detours Package](https://github.com/microsoft/Detours)  
Detours is a software package for monitoring and instrumenting API calls on Windows.  
### [VC-LTL - An elegant way to compile lighter binaries.](https://github.com/Chuyu-Team/VC-LTL5)  
VC-LTL is an open source CRT library based on the MS VCRT that reduce program binary size and say goodbye to Microsoft runtime DLLs, such as msvcr120.dll, api-ms-win-crt-time-l1-1-0.dll and other dependencies.  
### [Windows Implementation Libraries (WIL)](https://github.com/Microsoft/wil)  
The Windows Implementation Libraries (WIL) is a header-only C++ library created to make life easier for developers on Windows through readable type-safe C++ interfaces for common Windows coding patterns.  
