# Separator
[Go back to the previous level](../CONFIG.md)

## Description
> Note:   
> [Menu](../CONFIG.md) must have `EnableCustomRendering` set to 1 for the values on this page to be considered; otherwise, the values on this page will be ignored.

Storage location: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator`

## Values
Type: **DWORD**

> Inherits from [CustomRendering](../CustomRendering/CONFIG.md); likewise, it uses the values from there.

<table>
<tr>
<th>Name</th>
<th>Accepted Values</th>
<th>Description</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>Width</b></dt>
<dt>Default Value: 1000</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Controls the thickness of the separator line in the pop-up menu.</dt>
<dt>TranslucentFlyouts converts this value to device-independent pixels (DIP) by dividing it by 1000.</dt>
</td>
</tr>

</table>

## Reading Order
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\Separator`
3. Use default values
