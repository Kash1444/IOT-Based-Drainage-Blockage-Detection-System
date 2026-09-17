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
        # Blynk Cloud's documented external API uses the virtual-pin query
        # parameter (for example /external/api/get?token=...&v0).
        response = httpx.get(f"{self.base_url}/external/api/get", params={"token": self.token, "v": pin.lower()}, timeout=10)
        response.raise_for_status()
        value = response.json()
        if isinstance(value, (dict, list)):
            raise ValueError("Blynk returned a non-scalar datastream value")
        return value
