# Menu
[返回上一级](../CONFIG.md)
## 描述
存储位置：`HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu`  
有关的子项，见：
- [Animation](./Animation/CONFIG.md)  
- [DisabledHot](./DisabledHot/CONFIG.md)    
- [Focusing](./Focusing/CONFIG.md)    
- [Hot](./Hot/CONFIG.md)     
- [Separator](./Separator/CONFIG.md)    
## 值
类型: <b>DWORD</b>  
> 继承于[TranslucentFlyouts](../CONFIG.md)，同样使用其中的值
<table>
<tr>
<th>名称</th>
<th>可被接受的值</th>
<th>描述</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>NoSystemDropShadow</b></dt>
<dt>默认值: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>禁用: 0</dt>
<dt>启用: 1</dt>
</dl>
</td>
<td width="30%">
<dt>移除老式的系统阴影(SysShadow)。</dt>
<dt>通常来说Windows 11没有老式的系统阴影，除非你使用直角样式的菜单</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableImmersiveStyle</b></dt>
<dt>默认值: 1</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>禁用: 0</dt>
<dt>启用: 1</dt>
</dl>
</td>
<td width="30%">
<dt>统一弹出菜单的外观为现代化菜单。</dt>
<br>
<b>Windows 11：</b>强烈建议不要禁用该项
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableCustomRendering</b></dt>
<dt>默认值: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>禁用: 0</dt>
<dt>启用: 1</dt>
</dl>
</td>
<td width="30%">
<dt>开启自定义渲染模式。</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableFluentAnimation</b></dt>
<dt>默认值: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>禁用: 0</dt>
<dt>启用: 1</dt>
</dl>
</td>
<td width="30%">
<dt>为弹出菜单开启现代化的流利弹出动画。</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>NoModernAppBackgroundColor</b></dt>
<dt>默认值: 1</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>禁用: 0</dt>
<dt>启用: 1</dt>
</dl>
</td>
<td width="30%">
<dt>移除UWP (如照片、画图、截图工具、商店) 图标的背景颜色。</dt>
<br>
<b>v2.0.0起可用</b>
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
<dt>当该项存在时，移除某些图标特定的背景颜色 (0xAARRGGBB)。</dt>
<dt>移除进程会从图标的四个角开始向中心扩展，直至无法移除。</dt>
<br>
<b>v2.0.0起可用</b>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>ColorTreatAsTransparentThreshold</b></dt>
<dt>默认值: 50</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0-510</dt>
</dl>
</td>
<td width="30%">
<dt>与要移除背景颜色之间的色差阈值。当像素之间的色差小于该色差阈值时，该像素点会被置为透明。</dt>
<br>
<dt>色差公式: √[(a1 - a2) ^ 2 + (r1 - r2) ^ 2 + (g1 - g2) ^ 2 + (b1 - b2) ^ 2]</dt>
<br>
<b>v2.0.0起可用</b>
</td>
</tr>

</table>

## 读取顺序
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu`
3. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\` 
4. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\` 
5. 使用默认值