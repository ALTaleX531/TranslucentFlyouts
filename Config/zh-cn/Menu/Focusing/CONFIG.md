# Focusing
[返回上一级](../CONFIG.md)
## 描述
> 注意:   
> [Menu](../CONFIG.md)中的`EnableCustomRendering`必须为1，否则本页面的值将会被忽略。

存储位置：`HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing`   
## 值
类型: <b>DWORD</b>  
> 继承于[CustomRendering](../CustomRendering/CONFIG.md)，同样使用其中的值
<table>
<tr>
<th>名称</th>
<th>可被接受的值</th>
<th>描述</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>Width</b></dt>
<dt>默认值: 1000</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>控制弹出菜单聚焦矩形的边框粗细。</dt>
<dt>TranslucentFlyouts会将该值除以1000转换为设备无关像素(DIP)。</dt>
<br>
<b>Windows 10：</b>不支持该值
</td>
</tr>

</table>

## 读取顺序
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\Focusing`
3. 使用默认值