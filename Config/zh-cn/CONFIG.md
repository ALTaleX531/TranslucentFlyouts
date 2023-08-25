# TranslucentFlyouts
> 由于可配置的功能较多，所以该页面可能无法及时得到更新。  
因此我们呼吁大家一起完善这份文档，任何PR都是受欢迎的！
## 其它语言
[English](../en-us/CONFIG.md)  
[Deutsch](../de-de/CONFIG.md)  
## 描述
目标版本：
- V2.0.0-alpha.4
- V2.0.0
> 注意:   
> 不支持更早的版本，对于更新的版本而言，该页面及子页面所显示的内容可能已经过时。   

当前版本TranslucentFlyouts使用注册表储存配置信息。   
存储位置：`HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\`   
这里的值用于定义弹出菜单和下拉控件的通用外观，子项中与之相同的值将被优先读取。   
有关的子项，见：
- [Menu](./Menu/CONFIG.md)  
- [DropDown](./DropDown/CONFIG.md)  
- [Tooltip](./Tooltip/CONFIG.md)  
## 值
类型: <b>DWORD</b>
<table>

<tr>
<th>名称</th>
<th>可被接受的值</th>
<th>描述</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EffectType</b></dt>
<dt>默认值: 5</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>无: 0</dt>
<dt>完全透明: 1</dt>
<dt>纯色: 2</dt>
<dt>模糊: 3</dt>
<dt>亚克力: 4</dt>
<dt>现代化亚克力: 5</dt>
<dt>亚克力背景层: 6</dt>
<dt>云母背景层: 7</dt>
<dt>云母变体背景层: 8</dt>
</dl>
</td>
<td width="30%">
<dt>弹出控件使用的效果和背景类型。</dt>
<br>
<dt><b>Windows 10:  </b>不支持6、7、8，使用5等同于使用4</dt>
<dt><b>Windows 11 22H2+:  </b>不支持2、3</dt>
<b>Windows 11 (Build 22000 之前):  </b>不支持6、8
<dt><b>注意: </b>使用6、7、8时，流利菜单动画将无法正确渲染</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>CornerType</b></dt>
<dt>默认值: 3</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>不更改: 0</dt>
<dt>直角: 1</dt>
<dt>大圆角: 2</dt>
<dt>小圆角: 3</dt>
</dl>
</td>
<td width="30%">
<dt>弹出控件使用的边角类型。</dt>
<br>
<dt><b>Windows 10:  </b>不支持</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableDropShadow</b></dt>
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
<dt>开启额外带有阴影的边框。</dt>
<b>注意：</b>当且仅当EffectType为4或5，且CornerType为1时才可见
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>NoBorderColor</b></dt>
<dt>默认值: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>使用边框颜色: 0</dt>
<dt>不使用边框颜色: 1</dt>
</dl>
</td>
<td width="30%">
<dt>不渲染系统边框。</dt>
<br>
<b>Windows 10:  </b>仅支持移除弹出菜单的系统边框
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableThemeColorization</b></dt>
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
<dt>使用你当前的主题色作为系统边框颜色</dt>
<dt>DarkMode_BorderColor/LightMode_BorderColor值存在时，该选项将被忽略</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_BorderColor</b></dt>
<dt>默认值: 主题定义的值</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>深色模式下边框颜色 (AARRGGBB)。</dt>
<br>
<dt><b>Windows 10:  </b>仅支持覆盖弹出菜单的系统边框颜色</dt>
<dt><b>Windows 11:  </b>CornerType不为1时，Alpha通道将总是被忽略</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_BorderColor</b></dt>
<dt>默认值: 主题定义的值</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>浅色模式下边框颜色 (AARRGGBB)。</dt>
<br>
<dt><b>Windows 10:  </b>仅支持覆盖弹出菜单的系统边框颜色</dt>
<dt><b>Windows 11:  </b>使用圆角时，Alpha通道将总是被忽略</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_GradientColor</b></dt>
<dt>默认值: 0x412B2B2B</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>深色模式下叠加色的颜色 (AARRGGBB)，被EffectType使用。</dt>
<br>
<b>注意：</b>当EffectType为6、7、8时，该值将会被忽略
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_GradientColor</b></dt>
<dt>默认值: 0x9EDDDDDD</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>浅色模式下叠加色的颜色 (AARRGGBB)，被EffectType使用。</dt>
<br>
<b>注意：</b>当EffectType为6、7、8时，该值将会被忽略
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>Disabled</b></dt>
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
<dt>禁用TranslucentFlyouts。</dt>
<b>弹出菜单和下拉控件的半透明和动画效果将消失。</b>
</td>
</tr>

</table>

## 读取顺序
有时候部分的值可能未被创建，即尚不存在，此时TranslucentFlyouts将使用特定的读取顺序作为回退，尽可能读取到创建的值。   
你可以使用此特性创建适用于多个用户的统一配置
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\`
3. 使用默认值