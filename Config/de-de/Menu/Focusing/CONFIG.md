# Fokussieren
[Zurück zur vorherigen Ebene](../CONFIG.md)
## Beschreibung
> Hinweis:
> „EnableCustomRendering“ in [Menü](../CONFIG.md) muss 1 sein, sonst wird der Wert dieser Seite ignoriert.

Speicherort: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing`   
## Wert
Geben Sie ein: <b>DWORD</b>
> Geerbt von [CustomRendering](../CustomRendering/CONFIG.md), verwenden Sie auch den Wert
<table>
<tr>
<th>Name</th>
<th>Akzeptierte Werte</th>
<th>Beschreibung</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>Width</b></dt>
<dt>Standard: 1000</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> steuert die Randstärke des Fokusrechtecks ​​des Popup-Menüs. </dt>
<dt>TranslucentFlyouts dividiert den Wert durch 1000, um ihn in geräteunabhängige Pixel (DIP) umzuwandeln. </dt>
<br>
<b>Windows 10:</b> Dieser Wert wird nicht unterstützt
</td>
</tr>

</table>

## Lesereihenfolge
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\Focusing`
3. Verwenden Sie Standardwerte