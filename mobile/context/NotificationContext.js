import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';
import * as Notifications from 'expo-notifications';

const NotificationContext = createContext(null);

const SAMPLE_INCIDENTS = [
  {
    id: '1',
    title: 'Person Detected',
    camera: 'Front Entrance — Camera 1',
    timestamp: new Date(Date.now() - 5 * 60 * 1000),
    severity: 'high',
    videoUrl: 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerBlazes.mp4',
    description: 'LENS AI detected a person approaching the front entrance after hours.',
    read: false,
  },
  {
    id: '2',
    title: 'Motion Alert',
    camera: 'Parking Lot — Camera 3',
    timestamp: new Date(Date.now() - 23 * 60 * 1000),
    severity: 'medium',
    videoUrl: 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerEscapes.mp4',
    description: 'Unexpected motion detected in the parking lot outside of business hours.',
    read: false,
  },
  {
    id: '3',
    title: 'Person Detected',
    camera: 'Side Exit — Camera 2',
    timestamp: new Date(Date.now() - 2 * 60 * 60 * 1000),
    severity: 'low',
    videoUrl: 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerFun.mp4',
    description: 'Person detected near side exit during normal business hours.',
    read: true,
  },
  {
    id: '4',
    title: 'Unauthorized Access Attempt',
    camera: 'Server Room — Camera 4',
    timestamp: new Date(Date.now() - 6 * 60 * 60 * 1000),
    severity: 'high',
    videoUrl: 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerJoyrides.mp4',
    description: 'Multiple failed attempts to access the server room detected.',
    read: true,
  },
];

export function NotificationProvider({ children }) {
  const [incidents, setIncidents] = useState(SAMPLE_INCIDENTS);
  const [unreadCount, setUnreadCount] = useState(
    SAMPLE_INCIDENTS.filter(i => !i.read).length
  );

  useEffect(() => {
    setUnreadCount(incidents.filter(i => !i.read).length);
  }, [incidents]);

  useEffect(() => {
    const subscription = Notifications.addNotificationReceivedListener(notification => {
      const newIncident = {
        id: Date.now().toString(),
        title: notification.request.content.title || 'New Alert',
        camera: notification.request.content.subtitle || 'Unknown Camera',
        timestamp: new Date(),
        severity: notification.request.content.data?.severity || 'medium',
        videoUrl:
          notification.request.content.data?.videoUrl ||
          'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerBlazes.mp4',
        description: notification.request.content.body || 'New security alert detected by LENS.',
        read: false,
      };
      setIncidents(prev => [newIncident, ...prev]);
    });
    return () => Notifications.removeNotificationSubscription(subscription);
  }, []);

  const markAsRead = useCallback((id) => {
    setIncidents(prev =>
      prev.map(incident => (incident.id === id ? { ...incident, read: true } : incident))
    );
  }, []);

  const markAllAsRead = useCallback(() => {
    setIncidents(prev => prev.map(i => ({ ...i, read: true })));
  }, []);

  return (
    <NotificationContext.Provider value={{ incidents, unreadCount, markAsRead, markAllAsRead }}>
      {children}
    </NotificationContext.Provider>
  );
}

export function useNotifications() {
  const ctx = useContext(NotificationContext);
  if (!ctx) throw new Error('useNotifications must be used within NotificationProvider');
  return ctx;
}
