<h2>Programming / config</h2>

<p>
The configuration file defines Zigbee/logical mappings, known relays, event rules,
disabled time ranges, relay-state conditions, delayed events, labels, and global
actions such as <code>Calloff</code>.
</p>

<p>
Empty lines and lines starting with <code>#</code> are ignored.
</p>

<h3>Token format</h3>

<dl>
  <dt><code>Kaaa</code></dt>
  <dd>
    Logical/key event. <code>aaa</code> is a three-digit number:
    <code>K001</code> to <code>K999</code>.
  </dd>

  <dt><code>Rijj</code></dt>
  <dd>
    Relay identifier. <code>i</code> is the device number,
    <code>jj</code> is the relay number on that device.
    Example: <code>R101</code> means device <code>1</code>, relay <code>01</code>.
    Example: <code>R230</code> means device <code>2</code>, relay <code>30</code>.
  </dd>

  <dt><code>Ew</code></dt>
  <dd>
    System event. <code>w</code> is the event number.
  </dd>

  <dt><code>DHHMM,HHMM</code></dt>
  <dd>
    Disabled time range. During this range the rule is not executed.
    Example: <code>D2300,0630</code> disables the rule from 23:00 to 06:30.
  </dd>

  <dt><code>Tijj,s</code></dt>
  <dd>
    Relay-state test. <code>ijj</code> identifies the relay and
    <code>s</code> is the required state: <code>0</code> for off,
    <code>1</code> for on.
    Example: <code>T101,1</code> is true when <code>R101</code> is on.
  </dd>

  <dt><code>Sw,t</code></dt>
  <dd>
    Delayed event. <code>w</code> is the system event to generate and
    <code>t</code> is the delay in seconds.
    Example: <code>S100,10</code> generates <code>E100</code> after 10 seconds.
  </dd>

  <dt><code>Lname</code></dt>
  <dd>
    Optional descriptive label for the rule. The label must not contain spaces.
    It does not change execution logic; it is only used by diagnostic commands
    such as <code>showevents</code> and <code>showkmap</code>.
    Example: <code>Lingresso_anticantina</code>.
  </dd>
</dl>

<h3>System events</h3>

<table>
  <thead>
    <tr>
      <th>Event</th>
      <th>Meaning</th>
    </tr>
  </thead>
  <tbody>
    <tr><td><code>E0</code></td><td>Reserved for internal communication.</td></tr>
    <tr><td><code>E1</code></td><td>Every minute.</td></tr>
    <tr><td><code>E2</code></td><td>Every hour.</td></tr>
    <tr><td><code>E3</code></td><td>Every 10 minutes.</td></tr>
    <tr><td><code>E4</code></td><td>Every 30 minutes.</td></tr>
    <tr><td><code>E5</code></td><td>Every day at 00:00.</td></tr>
    <tr><td><code>E6</code></td><td>Every day at 06:00.</td></tr>
    <tr><td><code>E7</code></td><td>Every day at 12:00.</td></tr>
    <tr><td><code>E8</code></td><td>Every day at 18:00.</td></tr>
    <tr><td><code>E9</code></td><td>Sunrise.</td></tr>
    <tr><td><code>E10</code></td><td>Sunset.</td></tr>
    <tr><td><code>E11..E499</code></td><td>Free events, usable with <code>Cset</code> / <code>S</code>.</td></tr>
  </tbody>
</table>

<h3>Kmap mapping</h3>

<p>
<code>Kmap</code> maps an external event, for example a Zigbee event, to a
logical <code>Kaaa</code> event.
</p>

<pre><code>Kmap device action Kaaa</code></pre>

<p>Example:</p>

<pre><code>Kmap 0x5c0272fffe225ae5 1_single K001
Kmap 0x5c0272fffe225ae5 2_single K002
Kmap 0x5c0272fffe225ae5 3_double K006</code></pre>

<p>
A remote UDP command can then trigger the mapped event:
</p>

<pre><code>zigbee 0x5c0272fffe225ae5 1_single</code></pre>

<p>
The program looks up the pair <code>device + action</code> and internally
generates the matching <code>Kaaa</code> event.
</p>

<p>
Internally, the <code>Kmap</code> table is sorted by <code>device</code> and
<code>action</code> for binary search. The <code>showkmap</code> diagnostic
view is sorted by assigned <code>Kaaa</code>, then by <code>device</code>,
then by <code>action</code>, without changing the runtime lookup order.
</p>

<h3>Known relays / Rrange</h3>

<p>
<code>Rrange</code> declares the set of relays known by the system.
</p>

<pre><code>Rrange Raaa Rbbb</code></pre>

<p>Example:</p>

<pre><code>Rrange R101 R130
Rrange R201 R230
Rrange R301 R330</code></pre>

<p>
This list is used by commands such as <code>showon</code> and by
<code>Calloff</code>.
</p>

<h3>Rule format</h3>

<p>
Rules are made of a <code>C...</code> command, one or more triggers
(<code>K...</code> or <code>E...</code>), optional relays, and optional
<code>D</code>, <code>T</code>, <code>S</code>, or <code>L</code> tokens.
</p>

<p>
Token order is free: the program recognizes tokens by their first letter.
For readability, this format is recommended:
</p>

<pre><code>Command Trigger Relays [conditions] [delayed-events] [label]</code></pre>

<p>Examples:</p>

<pre><code>Con K001 R101 Llight_on
Coff K002 R101 Llight_off
Conoff K003 R101 R102 Lroom_toggle
Ccondon K004 R201 T101,1 Lconditional_on
Cset K005 S100,10 Ldelayed_event
Coff E100 R130 Ldelayed_off</code></pre>

<h3>Config commands</h3>

<h4><code>Con</code></h4>

<p>Turns on the listed relays.</p>

<pre><code>Con {Kaaa|Ew} {Rijj...} {DHHMM,HHMM...} {Lname}</code></pre>

<p>Example:</p>

<pre><code>Con K912 R201 R110 Lstudio_on</code></pre>

<h4><code>Coff</code></h4>

<p>Turns off the listed relays.</p>

<pre><code>Coff {Kaaa|Ew} {Rijj...} {DHHMM,HHMM...} {Lname}</code></pre>

<p>Example:</p>

<pre><code>Coff K922 R201 R110 Lstudio_off</code></pre>

<h4><code>Conoff</code></h4>

<p>
Toggles the listed relays. If the majority of them are currently on,
they are turned off. Otherwise, they are turned on.
</p>

<pre><code>Conoff {Kaaa|Ew} {Rijj...} {DHHMM,HHMM...} {Lname}</code></pre>

<p>Examples:</p>

<pre><code>Conoff K001 R203 R115 R113 Lingresso_anticantina
Conoff K002 R201 R110 Lstudio</code></pre>

<p>
With three relays, zero or one relay on means the future state will be on for
all listed relays; two or three relays on means the future state will be off
for all listed relays.
</p>

<h4><code>Ccondon</code></h4>

<p>
Turns on the listed relays only if all listed <code>T</code> tests are true.
</p>

<pre><code>Ccondon {Kaaa|Ew} {Rijj...} {DHHMM,HHMM...} {Tijj,s...} {Lname}</code></pre>

<p>Example:</p>

<pre><code>Ccondon K010 R201 T101,1 Lconditional_on</code></pre>

<h4><code>Ccondoff</code></h4>

<p>
Turns off the listed relays only if all listed <code>T</code> tests are true.
</p>

<pre><code>Ccondoff {Kaaa|Ew} {Rijj...} {DHHMM,HHMM...} {Tijj,s...} {Lname}</code></pre>

<p>Example:</p>

<pre><code>Ccondoff K011 R201 T101,0 Lconditional_off</code></pre>

<h4><code>Cset</code></h4>

<p>
Generates one or more delayed system events.
</p>

<pre><code>Cset {Kaaa|Ew} {Sw,t...} {DHHMM,HHMM...} {Lname}</code></pre>

<p>Example:</p>

<pre><code>Cset K918 S100,10 Lwater_open_timer
Coff E100 R130 Lwater_open_stop</code></pre>

<h4><code>Calloff</code></h4>

<p>
Turns off all currently-on relays declared with <code>Rrange</code>, except
the relays explicitly listed on the same rule.
</p>

<pre><code>Calloff {Kaaa|Ew} {Rijj...} {Lname}</code></pre>

<p>
In a <code>Calloff</code> rule, the listed <code>R...</code> relays are
<strong>exclusions</strong>, not targets.
</p>

<p>Example:</p>

<pre><code>Calloff K006 R129 R130 Lall_off_except_water</code></pre>

<p>
Meaning: when <code>K006</code> is received, turn off all known relays that
are currently on, except <code>R129</code> and <code>R130</code>.
</p>

<p>
Typical use: a reasoned “all off” command that keeps critical relays active,
such as water, services, safety, or infrastructure relays.
</p>

<h3>Runtime indexing</h3>

<p>
Rules are not searched by scanning the whole configuration at runtime.
During config loading, rules are indexed directly by their trigger:
</p>

<pre><code>ee[K] - rules associated with Kaaa
ex[E] - rules associated with Ew</code></pre>

<p>
When <code>K010</code> is generated, the program directly accesses the
rules associated with <code>ee[10]</code> and executes only that rule list.
If a rule schedules a delayed event, for example <code>S100,10</code>, the
program later generates <code>E100</code> and then executes the rules
associated with <code>ex[100]</code>.
</p>

<h3>Reload behavior</h3>

<p>
The configuration can be reloaded at runtime with the remote UDP
<code>reload</code> command.
</p>

<p>
The reload procedure deliberately works in this order:
</p>

<ol>
  <li>free the current configuration;</li>
  <li>clear pending delayed events;</li>
  <li>load the configuration file again from the beginning;</li>
  <li>stop loading at the first invalid line;</li>
  <li>report either success or the line number where loading stopped.</li>
</ol>

<p>
If the configuration is correct, the program reports that it has been fully
loaded. If an error is found, the program reports the failing line and keeps
only the configuration loaded up to the previous valid line.
</p>

<p>
The runtime log and process start time are not cleared by <code>reload</code>.
</p>

<h3>Complete config example</h3>

<pre><code># button mapping
Kmap 0x5c0272fffe225ae5 1_single K001
Kmap 0x5c0272fffe225ae5 2_single K002
Kmap 0x5c0272fffe225ae5 3_single K003
Kmap 0x5c0272fffe225ae5 3_double K006

# known relays
Rrange R101 R130
Rrange R201 R230
Rrange R301 R330

# entrance and antechamber lights
Con K911 R203 R115 R113 Lingresso_anticantina_on
Coff K921 R203 R115 R113 Lingresso_anticantina_off
Conoff K001 R203 R115 R113 Lingresso_anticantina

# study lights
Con K912 R201 R110 Lstudio_on
Coff K922 R201 R110 Lstudio_off
Conoff K002 R201 R110 Lstudio

# reasoned all-off
Calloff K006 R129 R130 Lall_off_except_water

# cabin lamp
Coff R114 R101 E9 Lcabin_lamp_sunrise
Con R114 R101 E10 Lcabin_lamp_sunset

# open main water for 10 seconds
Con K918 R130 Lopen_main_water
Cset K918 S100,10 Lopen_main_water_timer
Coff E100 R130 Lopen_main_water_stop

# close main water for 10 seconds
Con K928 R129 Lclose_main_water
Cset K928 S101,10 Lclose_main_water_timer
Coff E101 R129 Lclose_main_water_stop</code></pre>

<h2>Remote UDP commands</h2>

<p>
The program listens on IPv6 UDP port <code>55556</code>.
</p>

<p>
Every remote command must include the installation password using this format:
</p>

<pre><code>password command [arguments]</code></pre>

<table>
  <thead>
    <tr>
      <th>Command</th>
      <th>Arguments</th>
      <th>Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>status</code></td>
      <td></td>
      <td>
        Shows general status information, including current time, process start
        time, last reload time, reload status, sunrise/sunset, counters, and log size.
      </td>
    </tr>
    <tr>
      <td><code>seton</code></td>
      <td><code>Rabb</code></td>
      <td>Turns relay <code>Rabb</code> on.</td>
    </tr>
    <tr>
      <td><code>setoff</code></td>
      <td><code>Rabb</code></td>
      <td>Turns relay <code>Rabb</code> off.</td>
    </tr>
    <tr>
      <td><code>read</code></td>
      <td><code>Rabb</code></td>
      <td>Reads the state of relay <code>Rabb</code>.</td>
    </tr>
    <tr>
      <td><code>showon</code></td>
      <td></td>
      <td>Shows relays that are currently on among those declared with <code>Rrange</code>.</td>
    </tr>
    <tr>
      <td><code>zigbee</code></td>
      <td><code>device action</code></td>
      <td>Looks up <code>device + action</code> in <code>Kmap</code> and generates the mapped <code>Kaaa</code> event.</td>
    </tr>
    <tr>
      <td><code>showkmap</code></td>
      <td></td>
      <td>
        Shows the <code>Kmap</code> table sorted by assigned <code>Kaaa</code>.
        Under each mapping, it also prints the direct config rules associated
        with that <code>Kaaa</code>, if any.
      </td>
    </tr>
    <tr>
      <td><code>showevents</code></td>
      <td></td>
      <td>Shows all configured events and labels.</td>
    </tr>
    <tr>
      <td><code>inject</code></td>
      <td><code>Kaaa</code></td>
      <td>Manually injects a logical/key event.</td>
    </tr>
    <tr>
      <td><code>inject</code></td>
      <td><code>Ew</code></td>
      <td>Manually injects a system event.</td>
    </tr>
    <tr>
      <td><code>showlog</code></td>
      <td><code>[n]</code></td>
      <td>Shows the last <code>n</code> lines of the rotating log. Default is 10.</td>
    </tr>
    <tr>
      <td><code>reload</code></td>
      <td></td>
      <td>
        Frees the current configuration, clears pending delayed events, reloads
        the config file, and reports either success or the first failing line.
      </td>
    </tr>
    <tr>
      <td><code>quit</code></td>
      <td></td>
      <td>Saves the log and terminates the program.</td>
    </tr>
    <tr>
      <td><code>help</code></td>
      <td></td>
      <td>Shows remote command help.</td>
    </tr>
  </tbody>
</table>

<h3>UDP examples</h3>

<pre><code>echo "PASSWORD status" | nc -6u -w1 &lt;ipv6&gt; 55556

echo "PASSWORD read R101" | nc -6u -w1 &lt;ipv6&gt; 55556

echo "PASSWORD seton R101" | nc -6u -w1 &lt;ipv6&gt; 55556

echo "PASSWORD zigbee 0x5c0272fffe225ae5 1_single" | nc -6u -w1 &lt;ipv6&gt; 55556

echo "PASSWORD inject K006" | nc -6u -w1 &lt;ipv6&gt; 55556

echo "PASSWORD reload" | nc -6u -w1 &lt;ipv6&gt; 55556</code></pre>

<h2>Remote access</h2>

<p>
UDP access is allowed only from IPv6 addresses listed in:
</p>

<pre><code>/home/tools/misc/domotic.access</code></pre>

<p>
The file can contain IPv6 addresses or IPv6 prefixes.
</p>

<p>Examples:</p>

<pre><code>::1/128
2001:db8:1234::/64
fd00:1234::/64</code></pre>

<p>
Empty lines and lines starting with <code>#</code> are ignored.
</p>
