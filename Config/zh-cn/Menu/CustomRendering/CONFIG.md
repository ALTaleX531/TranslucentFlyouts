# CustomRendering
[返回上一级](../CONFIG.md)
## 描述
> 注意:   
> [Menu](../CONFIG.md)中的`EnableCustomRendering`必须为1，否则本页面的值将会被忽略。

有关的存储位置：
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\DisabledHot`   
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing`   
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Hot`   
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator`   
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
<dt><b>CornerRadius</b></dt>
<dt>默认值: 8</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>菜单项的圆角半径</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_Color</b></dt>
<dt>DisabledHot 默认值: 0</dt>
<dt>Hot 默认值: 0x41808080</dt>
<dt>Separator 默认值: 0x30D9D9D9</dt>
<dt>Focusing 默认值: 0xFFFFFFFF</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>深色模式下的颜色 (AARRGGBB)，被用于渲染边框或填充矩形。</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_Color</b></dt>
<dt>DisabledHot 默认值: 0</dt>
<dt>Hot 默认值: 0x30000000</dt>
<dt>Separator 默认值: 0x30262626</dt>
<dt>Focusing 默认值: 0xFF000000</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>浅色模式下的颜色 (AARRGGBB)，被用于渲染边框或填充矩形。</dt>
<br>
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
<dt>0: 禁用</dt>
<dt>1: 启用</dt>
</dl>
</td>
<td width="30%">
<dt>使用你当前的主题色覆盖LightMode_Color/DarkMode_Color中的值</dt>
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
<dt>0: 启用</dt>
<dt>1: 禁用</dt>
</dl>
</td>
<td width="30%">
<dt>禁用后该部分不采用自定义渲染</dt>
</td>
</tr>

</table>

## 读取顺序
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\*` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\*`
3. 使用默认值