import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';
import * as Notifications from 'expo-notifications';
import { authFetch } from '../utils/api';

const NotificationContext = createContext(null);

function mapIncident(raw) {
  return {
    id:          raw.id,
    title:       'Concealment Detected',
    camera:      raw.camera_id,
    timestamp:   new Date(raw.timestamp).toISOString(),
    severity:    'high',
    read:        raw.reviewed === 1 || raw.reviewed === true,
    description: `LENS AI detected suspicious concealment activity in ${raw.camera_id}.`,
  };
}

export function NotificationProvider({ children }) {
  const [incidents, setIncidents]   = useState([]);
  const [unreadCount, setUnreadCount] = useState(0);
  const [loading, setLoading]       = useState(true);

  const fetchIncidents = useCallback(async () => {
    try {
      const res = await authFetch('/incidents');
      if (!res.ok) return;
      const data = await res.json();
      const mapped = data.map(mapIncident);
      setIncidents(mapped);
      setUnreadCount(mapped.filter(i => !i.read).length);
    } catch (err) {
      console.error('Failed to fetch incidents:', err.message);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchIncidents();
  }, []);

  useEffect(() => {
    const subscription = Notifications.addNotificationReceivedListener(() => {
      fetchIncidents();
    });
    return () => Notifications.removeNotificationSubscription(subscription);
  }, []);

  const markAsRead = useCallback(async (id) => {
    try {
      await authFetch(`/incidents/${id}/review`, { method: 'PATCH' });
      setIncidents(prev =>
        prev.map(i => i.id === id ? { ...i, read: true } : i)
      );
      setUnreadCount(prev => Math.max(0, prev - 1));
    } catch (err) {
      console.error('Failed to mark as read:', err.message);
    }
  }, []);

  const markAllAsRead = useCallback(async () => {
    try {
      await Promise.all(
        incidents.filter(i => !i.read).map(i =>
          authFetch(`/incidents/${i.id}/review`, { method: 'PATCH' })
        )
      );
      setIncidents(prev => prev.map(i => ({ ...i, read: true })));
      setUnreadCount(0);
    } catch (err) {
      console.error('Failed to mark all as read:', err.message);
    }
  }, [incidents]);

  return (
    <NotificationContext.Provider value={{
      incidents,
      unreadCount,
      loading,
      markAsRead,
      markAllAsRead,
      refresh: fetchIncidents,
    }}>
      {children}
    </NotificationContext.Provider>
  );
}

export function useNotifications() {
  const ctx = useContext(NotificationContext);
  if (!ctx) throw new Error('useNotifications must be used within NotificationProvider');
  return ctx;
}
