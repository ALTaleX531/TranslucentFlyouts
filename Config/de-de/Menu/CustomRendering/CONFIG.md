# CustomRendering
[Zurück zur vorherigen Ebene](../CONFIG.md)
## Beschreibung
> Hinweis:
> `EnableCustomRendering` in [Menu](../CONFIG.md) muss 1 sein, sonst wird der Wert dieser Seite ignoriert.

Relevante Speicherorte:
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\DisabledHot`
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Focusing`
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Hot`
- `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Separator`
## Wert
Geben Sie ein: <b>DWORD</b>
<table>
<tr>
<th>Name</th>
<th>Akzeptierte Werte</th>
<th>Beschreibung</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>CornerRadius</b></dt>
<dt>Standard: 8</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Der Eckenradius des Menüelements</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_Color</b></dt>
<dt>DisabledHot Standard: 0</dt>
<dt>Hot-Standardwert: 0x41808080</dt>
<dt>Standardwert für Trennzeichen: 0x30D9D9D9</dt>
<dt>Fokussierungsstandard: 0xFFFFFFFF</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> Die Dunkelmodusfarbe (AARRGGBB), die zum Rendern von Rändern oder Füllen von Rechtecken verwendet wird. </dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_Color</b></dt>
<dt>DisabledHot Standard: 0</dt>
<dt>Hot-Standardwert: 0x30000000</dt>
<dt>Standardwert für Trennzeichen: 0x30262626</dt>
<dt>Fokussierungsstandard: 0xFF000000</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> Die Lichtmodusfarbe (AARRGGBB), die zum Rendern von Rändern oder Füllen von Rechtecken verwendet wird. </dt>
<br>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableThemeColorization</b></dt>
<dt>Standard: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0: deaktiviert</dt>
<dt>1: aktivieren</dt>
</dl>
</td>
<td width="30%">
<dt>Überschreiben Sie die Werte in LightMode_Color/DarkMode_Color mit Ihrer aktuellen Designfarbe</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>Disabled</b></dt>
<dt>Standard: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0: aktivieren</dt>
<dt>1: deaktiviert</dt>
</dl>
</td>
<td width="30%">
<dt>Wenn diese Option deaktiviert ist, verwendet dieser Teil kein benutzerdefiniertes Rendering</dt>
</td>
</tr>

</table>

## Lesereihenfolge
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\*`
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\*`
3. Verwenden Sie Standardwerte