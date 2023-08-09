# CustomRendering
[Go back to the previous level](../CONFIG.md)

## Description
> Note:   
> [Menu](../CONFIG.md) must have `EnableCustomRendering` set to 1 for the values on this page to be considered; otherwise, the values on this page will be ignored.

Related storage locations:
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\DisabledHot`   
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing`   
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Hot`   
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator`

## Values
Type: **DWORD**

<table>
<tr>
<th>Name</th>
<th>Accepted Values</th>
<th>Description</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>CornerRadius</b></dt>
<dt>Default Value: 8</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Corner radius of menu items.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_Color</b></dt>
<dt>DisabledHot Default Value: 0</dt>
<dt>Hot Default Value: 0x41808080</dt>
<dt>Separator Default Value: 0x30D9D9D9</dt>
<dt>Focusing Default Value: 0xFFFFFFFF</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Color in dark mode (AARRGGBB) used for rendering borders or filling rectangles.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_Color</b></dt>
<dt>DisabledHot Default Value: 0</dt>
<dt>Hot Default Value: 0x30000000</dt>
<dt>Separator Default Value: 0x30262626</dt>
<dt>Focusing Default Value: 0xFF000000</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Color in light mode (AARRGGBB) used for rendering borders or filling rectangles.</dt>
</td>
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
<dt>0: Disable</dt>
<dt>1: Enable</dt>
</dl>
</td>
<td width="30%">
<dt>Use your current theme color to override values in LightMode_Color/DarkMode_Color.</dt>
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
<dt>0: Enable</dt>
<dt>1: Disable</dt>
</dl>
</td>
<td width="30%">
<dt>Disable custom rendering for this section after enabling it.</dt>
</td>
</tr>

</table>

## Reading Order
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\*` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\*`
3. Use default values
