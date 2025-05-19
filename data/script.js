
// Universum + Netmode laden und anzeigen
fetch('/config.json')
  .then(res => res.json())
  .then(data => {
    document.getElementById('universe').value = data.universe;
    document.querySelectorAll('input[name="netmode"]').forEach(radio => {
      if (radio.value === data.netmode) radio.checked = true;
    });
  });

// Formular zum Speichern der Konfiguration
document.getElementById('settingsForm').addEventListener('submit', function(e) {
  e.preventDefault();
  const universe = document.getElementById('universe').value;
  const netmode = document.querySelector('input[name="netmode"]:checked').value;
  const formData = new URLSearchParams();
  formData.append('universe', universe);
  formData.append('netmode', netmode);
  fetch('/save', {
    method: 'POST',
    body: formData,
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
  }).then(res => res.text()).then(alert);
});

// LED-Testfunktionen
function testLed(color) {
  let query = "";
  if (color === "r") query = "?r";
  else if (color === "g") query = "?g";
  else if (color === "b") query = "?b";
  else query = "";
  fetch("/test" + query);
}
