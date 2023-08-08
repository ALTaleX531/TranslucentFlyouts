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
> 继承于[TranslucentFlyouts](..\CONFIG.md)，同样使用其中的值
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

</table>

## 读取顺序
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu`
3. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\` 
4. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\` 
5. 使用默认值