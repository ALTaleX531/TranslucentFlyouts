# Trennzeichen
[Zurück zur vorherigen Ebene](../CONFIG.md)
## Beschreibung
> Hinweis:
> `EnableCustomRendering` in [Menu](../CONFIG.md) muss 1 sein, sonst wird der Wert dieser Seite ignoriert.

Speicherort: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator`
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
<dt> steuert die Dicke der Teilungslinie des Popup-Menüs. </dt>
<dt>TranslucentFlyouts dividiert den Wert durch 1000, um ihn in geräteunabhängige Pixel (DIP) umzuwandeln. </dt>
</td>
</tr>

</table>

## Lesereihenfolge
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator`
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\Separator`
3. Verwenden Sie Standardwerte