# Animation
[Go back to the previous level](../CONFIG.md)

## Description
> Note:   
> Most of the values set on this page require `EnableFluentAnimation` in [Menu](../CONFIG.md) to be 1; otherwise, these values will be ignored.

Storage location: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Animation`

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
<dt><b>FadeOutTime</b></dt>
<dt>Default Value: 350</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Duration of the fade-out animation after interacting with a menu item (ms).</dt>
<br>
<dt><b>EnableFluentAnimation doesn't need to be 1.</b></dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>PopInTime</b></dt>
<dt>Default Value: 250</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Duration of the pop-in animation when the menu appears (ms).</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>FadeInTime</b></dt>
<dt>Default Value: 87</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Duration of the fade-in animation when the menu appears (ms).</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>PopInStyle</b></dt>
<dt>Default Value: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0: Slide Down (WinUI/UWP)</dt>
<dt>1: Ripple</dt>
<dt>2: Smooth Scroll</dt>
<dt>3: Smooth Zoom</dt>
</dl>
</td>
<td width="30%">
<dt>Style of the pop-in animation.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>StartRatio</b></dt>
<dt>Default Value: 50</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0-100</dt>
</dl>
</td>
<td width="30%">
<dt>Percentage (%) of the pop-in animation.</dt>
<dt>Increasing this value can reduce the visual perception of animation lag, but too high may weaken the visual effect of the animation; decreasing this value can enhance the visual experience of the animation, but too low may affect the interaction experience.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableImmediateInterupting</b></dt>
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
<dt>Allow users to immediately interrupt animations.</dt>
<dt>Enabling this option will immediately stop and end the Fluent animation when a menu item is selected/hovered with the mouse.</dt>
</td>
</tr>

</table>

## Reading Order
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Animation` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\Animation`
3. Use default values
