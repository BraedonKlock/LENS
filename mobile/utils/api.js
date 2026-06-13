import * as SecureStore from 'expo-secure-store';

export const API_URL = 'http://192.168.1.234:3000';

export async function authFetch(path, options = {}) {
  const token = await SecureStore.getItemAsync('lens_auth_token');
  const response = await fetch(`${API_URL}${path}`, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      ...(token ? { Authorization: `Bearer ${token}` } : {}),
      ...options.headers,
    },
  });
  return response;
}
