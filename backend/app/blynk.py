"""Small, opt-in Blynk Cloud adapter. It never enables live mode by accident."""
import os
import httpx

class BlynkAdapter:
    def __init__(self):
        self.token = os.getenv("BLYNK_TOKEN", "")
        self.base_url = os.getenv("BLYNK_BASE_URL", "https://blynk.cloud").rstrip("/")
    @property
    def configured(self):
        return bool(self.token)
    def read(self, pin: str):
        if not self.configured:
            raise RuntimeError("Blynk is not configured; set BLYNK_TOKEN")
        # Blynk Cloud's documented form is /external/api/get?token=...&v0.
        response = httpx.get(
            f"{self.base_url}/external/api/get",
            params=[("token", self.token), (pin.lower(), "")],
            timeout=10,
        )
        response.raise_for_status()
        value = response.text.strip()
        if not value:
            raise ValueError("Blynk returned an empty datastream value")
        return value

    def read_datastreams(self, pins=("V0", "V1", "V2", "V3", "V4", "V5", "V6", "V7")):
        """Read the ESP32's documented virtual pins without exposing the token."""
        return {pin: self.read(pin) for pin in pins}
