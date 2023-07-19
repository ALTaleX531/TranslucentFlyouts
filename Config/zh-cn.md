###  Other Languages
[English](../CONFIG.md)  
## 存储位置
TranslucentFlyouts将绝大部分信息储存在Windows注册表中，你可以通过访问```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\```找到它们。配置信息会根据功能进行分类，在这个路径下依次展开成相关的子分支

由于当前UI还没有开发出来，所以需要你手动进行修改，每一次更改都会立即生效
如果指定的键值不存在，你需要手动创建它们

## 外观设置
<details><summary><b>通用</b></summary>

### 
路径：```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts```  
> 注意：**如果```Menu```或```DropDown```下存在同名值，则会优先读取子分支的值，忽略该部分**  

以下为可被接受的值  

```EffectType```
```ini
# DWORD(32)值，控制效果类型，范围为0~7
# 0，禁用任何效果
# 1，透明，背景没有模糊等效果
# 2，纯色，不透明度将会被忽略
# 3，模糊，又称Aero
# 4，经典亚克力模糊
# 5，现代亚克力模糊，Windows 10表现同上，仅Windows 11可用
# ======= 以下的值仅Windows 11可用 ========
# 6，Acrylic，叠加色和不透明度会被忽略，由系统自动决定
# 7，Mica，叠加色和不透明度会被忽略，由系统自动决定，不推荐，看起来很丑
# 8，MicaAlt，叠加色和不透明度会被忽略，由系统自动决定，相比于Mica透明度更高，不推荐，也是看起来很丑
```
```EnableDropShadow```
```ini
# DWORD(32)值，控制是否启用阴影，设为1启用，0则禁用，推荐启用
```
```DarkMode_GradientColor```
```ini
# DWORD(32)值，控制暗模式下的叠加色（RGB）
```
```LightMode_GradientColor```
```ini
# DWORD(32)值，控制亮模式下的叠加色（RGB）
```
```DarkMode_Opacity```
```ini
# DWORD(32)值，控制暗模式下的不透明度（RGB）
```
```LightMode_Opacity```
```ini
# DWORD(32)值，控制亮模式下的不透明度（RGB）
```
```Disabled```
```ini
# DWORD(32)值，控制是否禁用
```
</details>

更改你的通用外观
<details><summary><b>下拉控件</b></summary>

### 
路径：```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\DropDown```  
以下为可被接受的值  

详见于```通用```
</details>

更改下拉控件的外观

<details><summary><b>菜单</b></summary>

### 
路径：```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu```  
以下为可被接受的值  

```NoSystemOutline```
```ini
# DWORD(32)值，控制是否去除系统绘制的边界矩形，设为1移除，0则不去除
# 设为1后，你可以自定义圆角类型(Windows 11)，改变甚至移除边框颜色(Windows 11)，
# 以及边界矩形的颜色(Windows 10/11)
```
```EnableImmersiveStyle```
```ini
# DWORD(32)值，控制是否使用现代化的菜单外观，设为1启用，0则禁用（默认情况下启用）
# 在Windows 10上，外观将尽可能与桌面右键的上下文菜单一致
# 在Windows 11上，用户不应该更改此值
```
```EnableCustomRendering```
```ini
# DWORD(32)值，控制是否使用自定义渲染，设为1启用，0则禁用（默认情况下禁用）
# 默认情况下使用主题位图和Theme API进行渲染，但有时候你可能觉得系统默认的主题太丑了
# 启用该项后，TranslucentFlyouts将采用Direct2D进行渲染，
# 这时候，你不仅可以自定义颜色和不透明度，还可以获得性能上的大大提升
# 在Windows 11上，用户可以更改此值以兼容StartAllBack
```

其余内容详见于```通用```
</details>

更改弹出菜单的大体外观

<details><summary><b>菜单详细项</b></summary>

### 
路径：```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\*```  
> ```*```有```Hot、DisabledHot、Focusing(仅Windows 11)、Border、Separator```  

以下为可被接受的值  

```DarkMode_GradientColor```
```ini
# DWORD(32)值，控制暗模式下的叠加色（RGB）
# Border分支下该值是边框颜色，其余分支仅在启用EnableCustomRendering后使用
```
```LightMode_GradientColor```
```ini
# DWORD(32)值，控制亮模式下的叠加色（RGB）
# Border分支下该值是边框颜色，其余分支仅在启用EnableCustomRendering后使用
```
```DarkMode_Opacity```
```ini
# DWORD(32)值，控制暗模式下的不透明度（RGB）
# Windows 11中，Border分支下该值不可用，其余分支仅在启用EnableCustomRendering后使用
```
```LightMode_Opacity```
```ini
# DWORD(32)值，控制亮模式下的不透明度（RGB）
# Windows 11中，Border分支下该值不可用，其余分支仅在启用EnableCustomRendering后使用
```
```Disabled```
```ini
# DWORD(32)值，控制是否禁用该部分的自定义渲染
# 例如你不希望分割线被自定义渲染接管，你可以将Separator下的此值设为1
# Border分支无视该值
```
```CornerRadius```
```ini
# DWORD(32)值，控制圆角半径大小，推荐为8
# 仅在启用EnableCustomRendering后使用
```
```EnableThemeColorization```
```ini
# DWORD(32)值，控制是否用当前主题色填充叠加色和不透明度
# 仅在启用EnableCustomRendering后使用
```
</details>

定义弹出菜单高亮项、禁用的高亮项，边框，聚焦等的外观  

<details><summary><b>较为特殊的菜单详细项</b></summary>

### 分割线
路径：```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator```  

以下为可被接受的值  

```SeparatorWidth```
```ini
# DWORD(32)值，控制分割线的粗细
# 该值的用法和名称可能会在未来被改变、删除，请尽量不要使用它
```

### 边框
路径：```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Border```  
由于历史遗留原因，Windows 11有两种边框，第一种边框Windows 10也有，通常来讲你只会看到第二种边框，并且它无法设置不透明度，为了设置的通用性，TranslucentFlyouts会同时更改两种边框的外观

以下为可被接受的值  

```NoBorderColor```
```ini
# DWORD(32)值，控制是否禁用边框颜色，设为1禁用，0为默认选项
# 启用后颜色和不透明度将会被忽略
# 仅适用于Windows 11
```
```CornerType```
```ini
# DWORD(32)值，控制边角类型
# 0，保持默认，使用小圆角
# 1，直角
# 2，大圆角
# 3，小圆角
# 仅适用于Windows 11
```

### 聚焦项
路径：```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing```  
在Windows 11上通过按下键盘的↓↑触发，Windows 10不可用

以下为可被接受的值  

```FocusingWidth```
```ini
# DWORD(32)值，控制聚焦矩形的线宽
# 该值的用法和名称可能会在未来被改变、删除，请尽量不要使用它
```

</details>

定义分割线，聚焦矩形的大小，以及边框的更多信息

<details><summary><b>工具条（工具提示）</b></summary>

### 
路径：```HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Tooltip```  
以下为可被接受的值  

**即将到来！**  
**COMING SOON!**
</details>

更改工具条的大体外观
