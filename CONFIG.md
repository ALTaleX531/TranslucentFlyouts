## Storage Location
TranslucentFlyouts stores the majority of its information in the Windows registry, which can be accessed at ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\```. Configuration information is categorized based on functionality and is expanded into relevant subkeys under this path.

Since the current UI has not been developed yet, you need to manually make modifications, and each change will take effect immediately.

## Appearance Settings
<details><summary><b>General</b></summary>

### 
Path: ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts```  
> Note: **If there are values with the same name under ```Menu``` or ```DropDown```, the values in the subkeys will be prioritized, and this part will be ignored.**

The following are the accepted values:

```EffectType```
```ini
# DWORD (32-bit) value that controls the type of effect, ranging from 0 to 7
# 0: Disable all effects
# 1: Transparent - no background blur or other effects
# 2: Solid color - opacity will be ignored
# 3: Blur - also known as Aero
# 4: Classic acrylic blur
# 5: Modern acrylic blur - same as Windows 10, available only in Windows 11
# ======= The following values are available only in Windows 11 ========
# 6: Acrylic - overlay color and opacity are ignored, determined automatically by the system
# 7: Mica - overlay color and opacity are ignored, determined automatically by the system, not recommended as it looks ugly
# 8: MicaAlt - overlay color and opacity are ignored, determined automatically by the system, higher opacity compared to Mica, not recommended as it looks ugly
```
```EnableDropShadow```
```ini
# DWORD (32-bit) value that controls whether the drop shadow is enabled, set to 1 to enable, 0 to disable (recommended to enable)
```
```DarkMode_GradientColor```
```ini
# DWORD (32-bit) value that controls the overlay color in dark mode (RGB)
```
```LightMode_GradientColor```
```ini
# DWORD (32-bit) value that controls the overlay color in light mode (RGB)
```
```DarkMode_Opacity```
```ini
# DWORD (32-bit) value that controls the opacity in dark mode (RGB)
```
```LightMode_Opacity```
```ini
# DWORD (32-bit) value that controls the opacity in light mode (RGB)
```
```Disabled```
```ini
# DWORD (32-bit) value that controls whether the feature is disabled
```
</details>

Modify your general appearance settings.
<details><summary><b>Dropdown Control</b></summary>

### 
Path: ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\DropDown```  
The following are the accepted values:

Refer to the ```General``` section for details.
</details>

Modify the appearance of the dropdown control.

<details><summary><b>Menu</b></summary>

### 
Path: ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu```  
The following are the accepted values:

```NoSystemOutline```
```ini
# DWORD (32-bit) value that controls whether to remove the system-drawn bounding rectangle, set to 1 to remove, 0 to keep
# After setting to 1, you can customize the corner type (Windows 11), change or remove the border color (Windows 11),
# and change the color of the bounding rectangle (Windows 10/11)
```
```EnableImmersiveStyle```
```ini
# DWORD (32-bit) value that controls whether to use a modern menu style, set to 1 to enable, 0 to disable (enabled by default)
# On Windows 10, the appearance will be as consistent as possible with the context menu of the desktop right-click
# On Windows 11, users should not change this value
```
```EnableCustomRendering```
```ini
# DWORD (32-bit) value that controls whether to use custom rendering, set to 1 to enable, 0 to disable (disabled by default)
# By default, TranslucentFlyouts uses theme bitmaps and the Theme API for rendering, but sometimes you may find the default system themes too ugly
# When enabled, TranslucentFlyouts will use Direct2D for rendering, allowing you to not only customize the color and opacity but also achieve significant performance improvements
# On Windows 11, users can change this value to ensure compatibility with StartAllBack
```

For other details, refer to the ```General``` section.
</details>

Modify the overall appearance of the popup menu.

<details><summary><b>Menu Items</b></summary>

### 
Path: ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\*```  
> ```*``` can be ```Hot```, ```DisabledHot```, ```Focusing``` (Windows 11 only), ```Border```, or ```Separator```

The following are the accepted values:

```DarkMode_GradientColor```
```ini
# DWORD (32-bit) value that controls the overlay color in dark mode (RGB)
# For the Border subkey, this value represents the border color; for other subkeys, it is only used when EnableCustomRendering is enabled
```
```LightMode_GradientColor```
```ini
# DWORD (32-bit) value that controls the overlay color in light mode (RGB)
# For the Border subkey, this value represents the border color; for other subkeys, it is only used when EnableCustomRendering is enabled
```
```DarkMode_Opacity```
```ini
# DWORD (32-bit) value that controls the opacity in dark mode (RGB)
# In Windows 11, this value is not available for the Border subkey; for other subkeys, it is only used when EnableCustomRendering is enabled
```
```LightMode_Opacity```
```ini
# DWORD (32-bit) value that controls the opacity in light mode (RGB)
# In Windows 11, this value is not available for the Border subkey; for other subkeys, it is only used when EnableCustomRendering is enabled
```
```Disabled```
```ini
# DWORD (32-bit) value that controls whether the custom rendering is disabled for this part
# For example, if you don't want the separator to be handled by custom rendering, you can set this value to 1 under the Separator subkey
# The Border subkey ignores this value
```
```CornerRadius```
```ini
# DWORD (32-bit) value that controls the corner radius, recommended value is 8
# Only used when EnableCustomRendering is enabled
```
```EnableThemeColorization```
```ini
# DWORD (32-bit) value that controls whether to use the current theme color to fill the overlay color and opacity
# Only used when EnableCustomRendering is enabled
```
</details>

Define the appearance of highlighted items, disabled highlighted items, borders, separators, etc., in the popup menu.

<details><summary><b>Special Menu Items</b></summary>

### Separator
Path: ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator```

The following are the accepted values:

```ini
# DWORD (32-bit) value that controls the thickness of the separator line
# The usage and name of this value may change or be removed in the future, so it is recommended not to use it
```

### Border
Path: ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Border```  
In Windows 11, there are two types of borders due to historical reasons. Usually, you will only see the second type of border, and it cannot have opacity settings. To ensure consistency in settings, TranslucentFlyouts will change the appearance of both types of borders.

The following are the accepted values:

```NoBorderColor```
```ini
# DWORD (32-bit) value that controls whether to disable the border color, set to 1 to disable, 0 for the default option
# When enabled, the color and opacity will be ignored
# Only applicable to Windows 11
```
```CornerType```
```ini
# DWORD (32-bit) value that controls the corner type
# 0: Keep the default, use small corners
# 1: Square corners
# 2: Large corners
# 3: Small corners
# Only applicable to Windows 11
```

### Focusing Item
Path: ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing```  
Triggered by the ↓↑ keys on the keyboard in Windows 11, not available in Windows 10.

The following are the accepted values:

```FocusingWidth```
```ini
# DWORD (32-bit) value that controls the line width of the focusing rectangle
# The usage and name of this value may change or be removed in the future, so it is recommended not to use it
```

</details>

Define the size of the separator, focusing rectangle, and more details of the border.

<details><summary><b>Toolbar (Tooltip)</b></summary>

### 
Path: ```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Tooltip```  
The following are the accepted values:

**Coming soon!**

</details>

Modify the overall appearance of the toolbar (tooltip).