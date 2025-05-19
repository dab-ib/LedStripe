window.onload = () => {
    fetch("/config.json")
      .then(res => res.json())
      .then(data => {
        document.getElementById("universe").value = data.universe;
        document.querySelector(`input[name="netmode"][value="${data.netmode}"]`).checked = true;
      });
  };
  
  document.getElementById("settingsForm").addEventListener("submit", (e) => {
    e.preventDefault();
    const universe = document.getElementById("universe").value;
    const netmode = document.querySelector('input[name="netmode"]:checked').value;
  
    fetch("/save", {
      method: "POST",
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ universe, netmode })
    })
    .then(() => alert("Gespeichert!"));
  });
  
  function testLed(color) {
    fetch("/test?" + color);
  }
  