# TranslucentFlyouts
> Due to the numerous configurable options, this page may not be updated promptly.
> Therefore, we appeal to everyone to improve this documentation together, and any PRs are welcome!

## Other languages
[简体中文](../zh-cn/CONFIG.md)   
[Deutsch](../de-de/CONFIG.md)   

## Description
Target version:
- V2.0.0-alpha.4
- V2.0.0
> Note:   
> Earlier versions are not supported, and the content displayed on this page and its subpages may be outdated for updated versions.

The current version of TranslucentFlyouts uses the registry to store configuration information.   
Storage location: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\`   
The values here are used to define the common appearance of pop-up menus and dropdown controls. Values in child items with the same name will be given priority.

Related child items can be found at:
- [Menu](./Menu/CONFIG.md)
- [DropDown](./DropDown/CONFIG.md)
- [Tooltip](./Tooltip/CONFIG.md)

## Values
Type: <b>DWORD</b>

<table>
<tr>
<th>Name</th>
<th>Accepted Values</th>
<th>Description</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EffectType</b></dt>
<dt>Default Value: 5</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>None: 0</dt>
<dt>Fully Transparent: 1</dt>
<dt>Solid Color: 2</dt>
<dt>Blurred: 3</dt>
<dt>Acrylic: 4</dt>
<dt>Modern Acrylic: 5</dt>
<dt>Acrylic Background Layer: 6</dt>
<dt>Mica Background Layer: 7</dt>
<dt>Mica Variant Background Layer: 8</dt>
</dl>
</td>
<td width="30%">
<dt>Effect and background type used for pop-up controls.</dt>
<br>
<dt><b>Windows 10:  </b>Not supported: 6, 7, 8; using 5 is equivalent to using 4</dt>
<dt><b>Windows 11 22H2+:  </b>Not supported: 2, 3</dt>
<b>Windows 11 (Before Build 22000):  </b>Not supported: 6, 8
<dt><b>Note: </b>When using 6, 7, 8, the Fluent Menu animation will not be rendered correctly</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>CornerType</b></dt>
<dt>Default Value: 3</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Don't Change: 0</dt>
<dt>Sharp Corner: 1</dt>
<dt>Large Round Corner: 2</dt>
<dt>Small Round Corner: 3</dt>
</dl>
</td>
<td width="30%">
<dt>Corner type used for pop-up controls.</dt>
<br>
<dt><b>Windows 10:  </b>Not supported</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableDropShadow</b></dt>
<dt>Default Value: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Disable: 0</dt>
<dt>Enable: 1</dt>
</dl>
</td>
<td width="30%">
<dt>Enable an additional border with a shadow.</dt>
<b>Note:</b> Only visible when EffectType is 4 or 5 and CornerType is 1.
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>NoBorderColor</b></dt>
<dt>Default Value: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Use Border Color: 0</dt>
<dt>Do Not Use Border Color: 1</dt>
</dl>
</td>
<td width="30%">
<dt>Do not render system borders.</dt>
<br>
<b>Windows 10:  </b>Only supports removing the system border of pop-up menus.</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableThemeColorization</b></dt>
<dt>Default Value: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Disable: 0</dt>
<dt>Enable: 1</dt>
</dl>
</td>
<td width="30%">
<dt>Use your current theme color as the system border color.</dt>
<dt>This option is ignored when DarkMode_BorderColor/LightMode_BorderColor values are present.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_BorderColor</b></dt>
<dt>Default Value: Theme-defined value</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Border color in dark mode (AARRGGBB).</dt>
<br>
<dt><b>Windows 10:  </b>Only supports overriding the system border color of pop-up menus.</dt>
<dt><b>Windows 11:  </b>The alpha channel will always be ignored when CornerType is not 1.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_BorderColor</b></dt>
<dt>Default Value: Theme-defined value</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Border color in light mode (AARRGGBB).</dt>
<br>
<dt><b>Windows 10:  </b>Only supports overriding the system border color of pop-up menus.</dt>
<dt><b>Windows 11:  </b>The alpha channel will always be ignored when using rounded corners.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_GradientColor</b></dt>
<dt>Default Value: 0x412B2B2B</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Overlay color in dark mode (AARRGGBB), used by EffectType.</dt>
<br>
<b>Note:</b> This value will be ignored when EffectType is 6, 7, or 8.
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_GradientColor</b></dt>
<dt>Default Value: 0x9EDDDDDD</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Overlay color in light mode (AARRGGBB), used by EffectType.</dt>
<br>
<b>Note:</b> This value will be ignored when EffectType is 6, 7, or 8.
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>Disabled</b></dt>
<dt>Default Value: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Disable: 0</dt>
<dt>Enable: 1</dt>
</dl>
</td>
<td width="30%">
<dt>Disable TranslucentFlyouts.</dt>
<b>The translucent and animation effects for pop-up menus and dropdown controls will be disabled.</b>
</td>
</tr>

</table>

## Reading Order
Sometimes, some values may not be created yet and do not exist. In this case, TranslucentFlyouts will use a specific reading order as a fallback to read existing values as much as possible.
You can use this feature to create a unified configuration that applies to multiple users.

1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\`
3. Use default values
