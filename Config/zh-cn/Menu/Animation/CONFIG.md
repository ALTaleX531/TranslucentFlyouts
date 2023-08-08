# Animation
[返回上一级](../CONFIG.md)
## 描述
> 注意:   
> 设置本页面中的大部分值都需要[Menu](../CONFIG.md)中的`EnableFluentAnimation`为1，否则这些值将会被忽略。

存储位置：`HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Animation`   
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
<dt><b>FadeOutTime</b></dt>
<dt>默认值: 350</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>菜单项交互后的淡出动画持续时间 (ms)。</dt>
<br>
<dt><b>EnableFluentAnimation可以不为1</b></dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>PopInTime</b></dt>
<dt>默认值: 250</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>弹出动画的弹出持续时间 (ms)。</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>FadeInTime</b></dt>
<dt>默认值: 87</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>弹出动画的淡入持续时间 (ms)。</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>PopInStyle</b></dt>
<dt>默认值: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0: 下滑 (WinUI/UWP)</dt>
<dt>1: 水波扩散</dt>
<dt>2: 平滑滚动</dt>
<dt>3: 平滑缩放</dt>
</dl>
</td>
<td width="30%">
<dt>弹出动画的样式。</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>StartRatio</b></dt>
<dt>默认值: 50</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0-100</dt>
</dl>
</td>
<td width="30%">
<dt>弹出动画的百分比 (%)。</dt>
<dt>提高该值可以减少动画滞后的视觉错觉，但过高会削弱动画的视觉效果；减少该值可以更好的享受动画的视觉体验，但过低可能会影响操作体验。</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableImmediateInterupting</b></dt>
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
<dt>允许用户立即打断动画。</dt>
<dt>启用该项后，当鼠标选中/悬浮在一个菜单项时会立刻中止并直接结束流利动画。</dt>
</td>
</tr>

</table>

## 读取顺序
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Animation` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\Animation`
3. 使用默认值