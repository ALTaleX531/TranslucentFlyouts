# Menu
[Zurück zur vorherigen Ebene](../CONFIG.md)
## Beschreibung
Speicherort: `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu`.
Zugehörige Unterpunkte finden Sie unter:
- [Animation](./Animation/CONFIG.md)
- [DisabledHot](./DisabledHot/CONFIG.md)
- [Fokussierung](./Focusing/CONFIG.md)
- [Hot](./Hot/CONFIG.md)
- [Trennzeichen](./Separator/CONFIG.md)
## Wert
Geben Sie ein: <b>DWORD</b>
> Von [TranslucentFlyouts](..\CONFIG.md) geerbt, verwenden Sie auch den Wert
<Tabelle>
<tr>
<th>Name</th>
<th>Akzeptierte Werte</th>
<th>Beschreibung</th>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>NoSystemDropShadow</b></dt>
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
<dt>Entfernen Sie den alten SysShadow. </dt>
<dt>Im Allgemeinen verfügt Windows 11 nicht über die Systemschatten im alten Stil, es sei denn, Sie verwenden ein Menü im rechteckigen Stil.</dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableImmersiveStyle</b></dt>
<dt>Standard: 1</dt>
</dl>
</td>
<td width="20%">
<dl>
<dt>deaktiviert: 0</dt>
<dt>Aktiviert: 1</dt>
</dl>
</td>
<td width="30%">
<dt>Vereinheitlicht das Erscheinungsbild des Popup-Menüs als modernes Menü. </dt>
<br>
<b>Windows 11:</b> Es wird dringend empfohlen, dies nicht zu deaktivieren
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableCustomRendering</b></dt>
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
<dt>Benutzerdefinierten Rendering-Modus aktivieren. </dt>
</td>
</tr>

<tr>
<td width="10%">
<dl>
<dt><b>EnableFluentAnimation</b></dt>
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
<dt> Ermöglicht moderne flüssige Popup-Animationen für Popup-Menüs. </dt>
</td>
</tr>

</table>

## Lesereihenfolge
1. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\Menu`
2. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\Menu`
3. `HKEY_CURRENT_USER\SOFTWARE\TranslucentFlyouts\`
4. `HKEY_LOCAL_MACHINE\SOFTWARE\TranslucentFlyouts\`
5. Verwenden Sie Standardwerte