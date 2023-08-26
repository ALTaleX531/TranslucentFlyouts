# Menu
[Go back to the previous level](../CONFIG.md)

## Description
Storage location: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu`  
Related child items can be found at:
- [Animation](./Animation/CONFIG.md)  
- [DisabledHot](./DisabledHot/CONFIG.md)    
- [Focusing](./Focusing/CONFIG.md)    
- [Hot](./Hot/CONFIG.md)     
- [Separator](./Separator/CONFIG.md)    

## Values
Type: **DWORD**
> Inherits from [TranslucentFlyouts](../CONFIG.md); uses the same values as described there.

<table>
<tr>
<th>Name</th>
<th>Accepted Values</th>
<th>Description</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>NoSystemDropShadow</b></dt>
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
<dt>Remove the old-style system shadow (SysShadow).</dt>
<dt>Generally, Windows 11 doesn't have old-style system shadows unless you're using menus with sharp corners.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableImmersiveStyle</b></dt>
<dt>Default Value: 1</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Disable: 0</dt>
<dt>Enable: 1</dt>
</dl>
</td>
<td width="30%">
<dt>Uniformly style pop-up menus as modern menus.</dt>
<br>
<b>Windows 11:</b> It's strongly recommended not to disable this option.
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableCustomRendering</b></dt>
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
<dt>Enable custom rendering mode.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableFluentAnimation</b></dt>
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
<dt>Enable modern Fluent pop-up animation for menus.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>NoModernAppBackgroundColor</b></dt>
<dt>Default Value: 1</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Disable: 0</dt>
<dt>Enable: 1</dt>
</dl>
</td>
<td width="30%">
<dt>Remove the background color of UWP icons (such as Photos, Paint, Snipping Tools, Store).</dt>
<br>
<b>Require v2.0.0</b>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>ColorTreatAsTransparent</b></dt>
<dt></dt>
</dl>
</td>
<td width="20%">
<dl>
<dt></dt>
</dl>
</td>
<td width="30%">
<dt>Removes specific background colors (0xAARRGGBB) of certain icons when this item exists.</dt>
<dt>The removal process expands from the four corners of the icon towards the center until the removal cannot be continued.</dt>
<br>
<b>Require v2.0.0</b>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>ColorTreatAsTransparentThreshold</b></dt>
<dt>Default Value: 50</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0-510</dt>
</dl>
</td>
<td width="30%">
<dt>The color difference threshold between the pixel and the background color to be removed. When the color difference between pixels is less than this color difference threshold, the pixel will be made transparent.</dt>
<br>
<dt>Equation: √[(a1 - a2) ^ 2 + (r1 - r2) ^ 2 + (g1 - g2) ^ 2 + (b1 - b2) ^ 2]</dt>
<br>
<b>Require v2.0.0</b>
</td>
</tr>

</table>

## Reading Order
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu`
3. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\` 
4. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\` 
5. Use default values
