const API_URL = process.env.VALLOX_API_URL || 'http://localhost:9000';
const API_TOKEN = process.env.VALLOX_API_TOKEN || '';

const headers: Record<string, string> = {
  'Authorization': `Bearer ${API_TOKEN}`,
  'Content-Type': 'application/json',
};

export async function getStatus(type: string = 'digit_vars'): Promise<Record<string, any>> {
  const url = `${API_URL}/api/vallox?action=get&type=${type}&token=${API_TOKEN}`;
  const res = await fetch(url, { cache: 'no-store' });

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
  const url = `${API_URL}/api/vallox?action=set&type=${type}&variable=${variable}&value=${value}&token=${API_TOKEN}`;
  const res = await fetch(url, { cache: 'no-store' });

  if (!res.ok) {
    throw new Error(`Vallox API error: ${res.status}`);
  }

  return res.json();
}
