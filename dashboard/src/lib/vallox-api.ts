const API_URL = process.env.VALLOX_API_URL || 'http://91.157.190.137:9000';
const API_TOKEN = process.env.VALLOX_API_TOKEN || 'huuhaa';

const headers: Record<string, string> = {
  'Authorization': `Bearer ${API_TOKEN}`,
  'Content-Type': 'application/json',
};

export async function getStatus(type: string = 'digit_vars'): Promise<Record<string, any>> {
  const url = `${API_URL}/api/vallox/status?type=${type}`;
  const res = await fetch(url, { headers, cache: 'no-store' });

  if (!res.ok) {
    throw new Error(`Vallox API error: ${res.status}`);
  }

  return res.json();
}

export async function setVariable(
  type: string,
  variable: string,
  value: number
): Promise<Record<string, any>> {
  const url = `${API_URL}/api/vallox/control`;
  const res = await fetch(url, {
    method: 'POST',
    headers,
    body: JSON.stringify({ type, variable, value }),
    cache: 'no-store',
  });

  if (!res.ok) {
    throw new Error(`Vallox API error: ${res.status}`);
  }

  const ret = await res.json();
  return ret;
}
