# TranslucentFlyouts
> Aufgrund der großen Anzahl konfigurierbarer Funktionen kann es sein, dass diese Seite nicht rechtzeitig aktualisiert wird.
Deshalb rufen wir alle dazu auf, dieses Dokument gemeinsam zu verbessern, und jede PR ist willkommen!
## andere Sprachen
[English](../en-us/CONFIG.md)   
[简体中文](../zh-cn/CONFIG.md)   
## Guide
Zielversion:
- V2.0.0-alpha.4
- V2.0.x
- V2.1.0
> Notiz:   
> Frühere Versionen werden nicht unterstützt und der auf dieser Seite und den Unterseiten angezeigte Inhalt ist für neuere Versionen möglicherweise veraltet.  

Die aktuelle Version von TranslucentFlyouts verwendet die Registrierung zum Speichern von Konfigurationsinformationen.
Speicherort: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\`

Der Wert hier wird verwendet, um das gemeinsame Erscheinungsbild des Popup-Menüs und des Dropdown-Steuerelements zu definieren, und derselbe Wert im Unterelement wird zuerst gelesen.
Zugehörige Unterpunkte finden Sie unter:

- [Menu](./Menu/CONFIG.md)  
- [DropDown](./DropDown/CONFIG.md)  
- [Tooltip](./Tooltip/CONFIG.md)  

## Wert
Typ: <b>DWORD</b>
<table>

<tr>
<th>Name</th>
<th>Akzeptierte Werte</th>
<th>Beschreibung</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EffectType</b></dt>
<dt>Standard: 5</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Keine: 0</dt>
<dt>Vollständig transparent: 1</dt>
<dt>Volltonfarben: 2</dt>
<dt>Unschärfe: 3</dt>
<dt>Acryl: 4</dt>
<dt>Modernes Acryl: 5</dt>
<dt>Acryl-Hintergrundebenen: 6</dt>
<dt>Glimmer-Hintergrundebenen: 7</dt>
<dt>Hintergrundebenen der Glimmervariante: 8</dt>
</dl>
</td>
<td width="30%">
<dt> Der vom Popup-Steuerelement verwendete Effekt- und Hintergrundtyp. </dt>
<br>
<dt><b>Windows 10: </b> unterstützt 6, 7, 8 nicht, die Verwendung von 5 entspricht der Verwendung von 4</dt>
<dt><b>Windows 11 22H2+: </b>2, 3 nicht unterstützt</dt>
<b>Windows 11 (vor Build 22000): </b> unterstützt 6, 8 nicht
<dt><b>Hinweis: </b>Bei Verwendung von 6, 7, 8 wird die Fluent-Menüanimation nicht korrekt gerendert</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>CornerType</b></dt>
<dt>Standard: 3</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Nicht ändern: 0</dt>
<dt>Rechter Winkel: 1</dt>
<dt>Großes Filet: 2</dt>
<dt>Kleine Filets: 3</dt>
</dl>
</td>
<td width="30%">
<dt> Der vom Popup-Steuerelement verwendete Eckentyp. </dt>
<br>
<dt><b>Windows 10: </b>nicht unterstützt</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableDropShadow</b></dt>
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
<dt> Aktiviert zusätzliche Ränder mit Schatten. </dt>
<b>Hinweis:</b> Nur sichtbar, wenn EffectType 4 oder 5 und CornerType 1 ist
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>NoBorderColor</b></dt>
<dt>Standard: 0</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>Rahmenfarbe verwenden: 0</dt>
<dt>Keine Rahmenfarbe verwenden: 1</dt>
</dl>
</td>
<td width="30%">
<dt> Systemränder nicht rendern. </dt>
<br>
<b>Windows 10: </b> unterstützt nur das Entfernen von Systemrändern für Popup-Menüs
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
<dt>deaktiviert: 0</dt>
<dt>Aktiviert: 1</dt>
</dl>
</td>
<td width="30%">
<dt>Verwenden Sie Ihre aktuelle Designfarbe als Systemrahmenfarbe</dt>
<dt>Wenn der Wert DarkMode_BorderColor/LightMode_BorderColor vorhanden ist, wird diese Option ignoriert</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_BorderColor</b></dt>
<dt>Standard: themendefinierter Wert</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> Randfarbe im Modus "Dunkel" (AARRGGBB). </dt>
<br>
<dt><b>Windows 10: </b>Unterstützt nur Systemrahmenfarben, die Popup-Menüs überschreiben</dt>
<dt><b>Windows 11: </b>Wenn CornerType nicht 1 ist, wird der Alpha-Kanal immer ignoriert</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_BorderColor</b></dt>
<dt>Standard: themendefinierter Wert</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> Randfarbe im Modus "Hell" (AARRGGBB). </dt>
<br>
<dt><b>Windows 10: </b>Unterstützt nur Systemrahmenfarben, die Popup-Menüs überschreiben</dt>
<dt><b>Windows 11: </b>Bei Verwendung abgerundeter Ecken wird der Alphakanal immer ignoriert</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>DarkMode_GradientColor</b></dt>
<dt>Standard: 0x412B2B2B</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt> Überlagerungsfarbe im Dunkelmodus (AARRGGBB), verwendet von EffectType. </dt>
<br>
<b>Hinweis:</b> Wenn EffectType 6, 7, 8 ist, wird dieser Wert ignoriert
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>LightMode_GradientColor</b></dt>
<dt>Standard: 0x9EDDDDDD</dt>
</dl>
</td>
<td width="20%">
<dl>
</dl>
</td>
<td width="30%">
<dt>Die Farbe der Überlagerungsfarbe im Modus "Hell" (AARRGGBB), die von EffectType verwendet wird. </dt>
<br>
<b>Hinweis:</b> Wenn EffectType 6, 7, 8 ist, wird dieser Wert ignoriert
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
<dt>deaktiviert: 0</dt>
<dt>Aktiviert: 1</dt>
</dl>
</td>
<td width="30%">
<dt>TranslucentFlyouts deaktivieren. </dt>
<b>Transluzenz- und Animationseffekte für Popup-Menüs und Dropdown-Steuerelemente verschwinden. </b>
</td>
</tr>

</table>

## Reihenfolge lesen
Manchmal werden einige Werte möglicherweise nicht erstellt, das heißt, sie sind noch nicht vorhanden. Zu diesem Zeitpunkt verwendet TranslucentFlyouts eine bestimmte Lesereihenfolge als Fallback, um die erstellten Werte so weit wie möglich zu lesen.
Mit dieser Funktion können Sie eine einheitliche Konfiguration erstellen, die für mehrere Benutzer gilt
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\` 
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\`
3. verwende den Standard