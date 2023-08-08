#Animation
[Zurück zur vorherigen Ebene](../CONFIG.md)
## Beschreibung
> Hinweis:
> Das Festlegen der meisten Werte auf dieser Seite erfordert, dass „EnableFluentAnimation“ in [Menu](../CONFIG.md) 1 ist, andernfalls werden diese Werte ignoriert.

Speicherort: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Animation`   
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
<dt><b>FadeOutTime</b></dt>
<dt>Standard: 350</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> Die Dauer (ms) der Ausblendanimation nach einer Menüelementinteraktion. </dt>
<br>
<dt><b>EnableFluentAnimation darf nicht 1 sein</b></dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>PopInTime</b></dt>
<dt>Standard: 250</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> Popup-Dauer (ms) für die Popup-Animation. </dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>FadeInTime</b></dt>
<dt>Standard: 87</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> Die Einblenddauer (ms) der Popup-Animation. </dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>PopInStyle</b></dt>
<dt>Standard: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0: Ausgefallen (WinUI/UWP)</dt>
<dt>1: Wasserwellendiffusion</dt>
<dt>2: Reibungsloses Scrollen</dt>
<dt>3: sanfter Zoom</dt>
</dl>
</td>
<td width="30%">
<dt> Stil für die Popup-Animation. </dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>StartRatio</b></dt>
<dt>Standard: 50</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>0-100</dt>
</dl>
</td>
<td width="30%">
<dt>Prozentsatz (%) der Popup-Animation. </dt>
<dt> Durch Erhöhen dieses Werts kann die visuelle Illusion einer Animationsverzögerung verringert werden. Ein zu hoher Wert schwächt jedoch den visuellen Effekt der Animation. Wenn Sie diesen Wert verringern, können Sie das visuelle Erlebnis der Animation besser genießen, ein zu niedriger Wert kann jedoch das Betriebserlebnis beeinträchtigen. </dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableImmediateInterupting</b></dt>
<dt>Standard: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>deaktiviert: 0</dt>
<dt>Aktiviert: 1</dt>
</dl>
</td>
<td width="30%">
<dt> Ermöglicht dem Benutzer, die Animation sofort zu unterbrechen. </dt>
<dt> Nach der Aktivierung dieser Option wird die fließende Animation sofort angehalten und beendet, wenn die Maus einen Menüpunkt auswählt/bewegt. </dt>
</td>
</tr>

</table>

## Lesereihenfolge
1. 1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu\Animation` 
2. 2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu\Animation`
3. Verwenden Sie Standardwerte