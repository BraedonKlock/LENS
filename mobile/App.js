import React, { useEffect, useRef } from 'react';
import { View } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { StatusBar } from 'expo-status-bar';
import * as Notifications from 'expo-notifications';
import * as Device from 'expo-device';
import { Platform } from 'react-native';
import { Ionicons } from '@expo/vector-icons';

import HomeScreen from './screens/HomeScreen';
import IncidentsScreen from './screens/IncidentsScreen';
import IncidentDetailScreen from './screens/IncidentDetailScreen';
import { NotificationProvider, useNotifications } from './context/NotificationContext';
import NotificationBell from './components/NotificationBell';

Notifications.setNotificationHandler({
  handleNotification: async () => ({
    shouldShowAlert: true,
    shouldPlaySound: true,
    shouldSetBadge: true,
  }),
});

const Tab = createBottomTabNavigator();
const Stack = createNativeStackNavigator();

function IncidentsStack() {
  return (
    <Stack.Navigator screenOptions={{ headerShown: false }}>
      <Stack.Screen name="IncidentsList" component={IncidentsScreen} />
      <Stack.Screen name="IncidentDetail" component={IncidentDetailScreen} />
    </Stack.Navigator>
  );
}

function Tabs() {
  const { unreadCount } = useNotifications();

  return (
    <Tab.Navigator
      screenOptions={{
        tabBarStyle: {
          backgroundColor: '#0D1117',
          borderTopColor: '#21262D',
          paddingBottom: 6,
          paddingTop: 4,
          height: 62,
        },
        tabBarActiveTintColor: '#58A6FF',
        tabBarInactiveTintColor: '#8B949E',
        headerStyle: { backgroundColor: '#0D1117' },
        headerTintColor: '#E6EDF3',
        headerTitleStyle: { fontWeight: 'bold' },
        headerShadowVisible: false,
      }}
    >
      <Tab.Screen
        name="Home"
        component={HomeScreen}
        options={{
          title: 'LENS Monitor',
          tabBarLabel: 'Dashboard',
          tabBarIcon: ({ color, size }) => (
            <Ionicons name="grid-outline" size={size} color={color} />
          ),
          headerRight: () => (
            <View style={{ marginRight: 8 }}>
              <NotificationBell />
            </View>
          ),
        }}
      />
      <Tab.Screen
        name="Incidents"
        component={IncidentsStack}
        options={{
          title: 'Incidents',
          tabBarLabel: 'Incidents',
          headerShown: false,
          tabBarIcon: ({ color, size }) => (
            <Ionicons name="warning-outline" size={size} color={color} />
          ),
          tabBarBadge: unreadCount > 0 ? unreadCount : undefined,
          tabBarBadgeStyle: { backgroundColor: '#FF4444', fontSize: 10 },
        }}
      />
    </Tab.Navigator>
  );
}

export default function App() {
  const notifListener = useRef();
  const responseListener = useRef();

  useEffect(() => {
    registerForNotifications();

    notifListener.current = Notifications.addNotificationReceivedListener(n => {
      console.log('Notification received:', n.request.content.title);
    });
    responseListener.current = Notifications.addNotificationResponseReceivedListener(r => {
      console.log('Notification tapped:', r.notification.request.content.title);
    });

    return () => {
      Notifications.removeNotificationSubscription(notifListener.current);
      Notifications.removeNotificationSubscription(responseListener.current);
    };
  }, []);

  return (
    <SafeAreaProvider>
      <NotificationProvider>
        <NavigationContainer>
          <StatusBar style="light" />
          <Tabs />
        </NavigationContainer>
      </NotificationProvider>
    </SafeAreaProvider>
  );
}

async function registerForNotifications() {
  if (Platform.OS === 'android') {
    await Notifications.setNotificationChannelAsync('lens-alerts', {
      name: 'LENS Security Alerts',
      importance: Notifications.AndroidImportance.MAX,
      vibrationPattern: [0, 250, 250, 250],
    });
  }

  if (!Device.isDevice) return;

  const { status: existing } = await Notifications.getPermissionsAsync();
  if (existing !== 'granted') {
    const { status } = await Notifications.requestPermissionsAsync();
    if (status !== 'granted') {
      console.log('Notification permission denied');
    }
  }
}
